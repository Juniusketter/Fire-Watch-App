"""
FireWatch Backend Server
========================
Flask REST API server for the FireWatch NFPA 10 fire extinguisher
compliance tracking platform.

Architecture:
    - This file serves as both the REST API and static file server
    - All data is stored in a single SQLite database (FireWatch.db)
    - The same database is shared with the Qt C++ desktop application
    - Multi-tenant: every data table includes org_id for organization isolation
    - Platform Super Admin: separate authentication for owner-level control

Endpoints (41 total):
    Authentication:  /api/login, /api/register, /api/platform/login
    Core CRUD:       /api/companies, /api/buildings, /api/extinguishers,
                     /api/assignments, /api/reports, /api/users
    Features:        /api/search, /api/upload, /api/stats, /api/notifications
    User Settings:   /api/users/:id/preferences, /api/users/:id/password
    Platform Admin:  /api/platform/organizations, /api/platform/settings,
                     /api/platform/audit

Security:
    - Passwords are SHA-256 hashed client-side before transmission
    - Server stores and compares hashes, never sees plaintext
    - Platform password configurable via FW_PLATFORM_HASH env var
    - All queries scoped by org_id for multi-tenant isolation

Requirements:
    pip install flask

Usage:
    python server.py

Then open your browser to:
    http://localhost:5000
"""

from flask import Flask, request, jsonify, send_from_directory
import sqlite3
import os
import hashlib
import logging
import uuid
import smtplib
import ssl
import secrets
from datetime import datetime, timedelta
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# Silence Flask's per-request log lines (GET /api/... 200)
# Errors and startup messages still print normally
logging.getLogger("werkzeug").setLevel(logging.ERROR)

def hash_password(pw: str) -> str:
    """SHA-256 hash a password string, returning a 64-char hex digest.
    This must match Qt's QCryptographicHash::Sha256 and the web frontend's
    crypto.subtle.digest('SHA-256', ...) so all three platforms produce
    identical hashes for the same password."""
    return hashlib.sha256(pw.encode()).hexdigest()

app = Flask(__name__)

# ── Paths ──────────────────────────────────────────────────────────────────────
# BASE_DIR: root of the project (where server.py lives)
# DB_PATH:  path to the shared SQLite database
# UI_DIR:   path to the web frontend (index.html served from here)
# UPLOAD_DIR: where inspection photos are stored (UUID-renamed files)
BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
DB_PATH     = os.path.join(BASE_DIR, "src", "database", "FireWatch.db")
UI_DIR      = os.path.join(BASE_DIR, "src", "frontend", "Fire-Watch-UI")
UPLOAD_DIR      = os.path.join(BASE_DIR, "src", "uploads")
BACKGROUND_DIR  = os.path.join(BASE_DIR, "src", "backgrounds")
os.makedirs(UPLOAD_DIR,     exist_ok=True)
os.makedirs(BACKGROUND_DIR, exist_ok=True)

# Allowed file extensions for photo uploads (validated on upload)
ALLOWED_EXTENSIONS = {"jpg", "jpeg", "png", "gif", "webp", "heic", "heif"}
# Allowed extensions for platform background images
BACKGROUND_EXTENSIONS = {"jpg", "jpeg", "png", "webp", "svg", "gif"}

# ── Platform Super Admin ──────────────────────────────────────────────────────
# The platform password is separate from org-level user accounts.
# Default: FireWatch2026! (hardcoded so teammates and professor can access)
# Override: set FW_PLATFORM_HASH environment variable on Render or locally
PLATFORM_PASSWORD_HASH = os.environ.get(
    "FW_PLATFORM_HASH",
    hashlib.sha256("FireWatch2026!".encode()).hexdigest()
)

# ── Email configuration ────────────────────────────────────────────────────────
# Set these environment variables on Render (or locally in your shell):
#   GMAIL_USER         — the Gmail address to send from  e.g. firewatch.app@gmail.com
#   GMAIL_APP_PASSWORD — the 16-char App Password from Google Account → Security
# If either is missing, email features degrade gracefully (log warning, return error).
GMAIL_USER     = os.environ.get("GMAIL_USER",         "")
GMAIL_APP_PASS = os.environ.get("GMAIL_APP_PASSWORD", "")
APP_BASE_URL   = os.environ.get("APP_BASE_URL",       "http://localhost:5000")

# ── DB helper ──────────────────────────────────────────────────────────────────
def get_db():
    """Open a connection to the SQLite database.
    row_factory=sqlite3.Row makes rows behave like dicts (access by column name)."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# ── Startup migrations — run once on server start ──────────────────────────────
def run_migrations():
    """Auto-create and update database tables on every server start.
    
    This function handles all schema evolution so teammates don't need to
    manually run SQL scripts. Each migration checks if a column/table exists
    before altering, making it safe to run repeatedly (idempotent).
    
    Migration order:
    1. Create core tables (Organizations, PlatformSettings, PlatformAuditLog)
    2. Add columns to existing tables (org_id, due_date, photo_path, etc.)
    3. Create Companies and Buildings tables (Sprint 3)
    4. Add NFPA 10 compliance columns to Extinguishers (10 fields)
    5. Add last_inspection_date to Extinguishers (Sprint 4)
    6. Backfill last_report_id and last_inspection_date from existing Reports
    7. Seed default organization for pre-existing data (one-time)
    """
    with get_db() as conn:

        tables = [r[0] for r in conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table'").fetchall()]

        # ── Organizations table ────────────────────────────────────────────
        if "Organizations" not in tables:
            conn.execute("""
                CREATE TABLE Organizations (
                    org_id       INTEGER PRIMARY KEY AUTOINCREMENT,
                    name         TEXT NOT NULL,
                    address      TEXT,
                    city         TEXT,
                    state        TEXT,
                    zip          TEXT,
                    phone        TEXT,
                    contact_name TEXT,
                    created_at   TEXT DEFAULT (datetime('now'))
                )
            """)
            conn.commit()

        # ── Add platform_status to Organizations ────────────────────────────
        org_cols = [r[1] for r in conn.execute("PRAGMA table_info(Organizations)").fetchall()]
        if "platform_status" not in org_cols:
            conn.execute("ALTER TABLE Organizations ADD COLUMN platform_status TEXT DEFAULT 'approved'")
            conn.commit()
            print("  [Migration] Added Organizations.platform_status")

        # ── Platform Settings table ─────────────────────────────────────────
        if "PlatformSettings" not in tables:
            conn.execute("""
                CREATE TABLE PlatformSettings (
                    key   TEXT PRIMARY KEY,
                    value TEXT
                )
            """)
            conn.execute("INSERT INTO PlatformSettings (key, value) VALUES ('require_approval', 'true')")
            conn.commit()
            print("  [Migration] Created PlatformSettings table")

        # ── Platform Audit Log table ────────────────────────────────────────
        if "PlatformAuditLog" not in tables:
            conn.execute("""
                CREATE TABLE PlatformAuditLog (
                    log_id     INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp  TEXT DEFAULT (datetime('now')),
                    actor      TEXT NOT NULL,
                    action     TEXT NOT NULL,
                    target     TEXT,
                    details    TEXT
                )
            """)
            conn.commit()
            print("  [Migration] Created PlatformAuditLog table")

        # ── Users ──────────────────────────────────────────────────────────
        user_cols = [r[1] for r in conn.execute("PRAGMA table_info(Users)").fetchall()]
        if "preferences" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN preferences TEXT DEFAULT '{}'")
        if "org_id" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN org_id INTEGER REFERENCES Organizations(org_id)")
        # PII fields — Sprint 5. Idempotent: safe to run on every server start.
        if "first_name" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN first_name TEXT DEFAULT ''")
            print("  [Migration] Added Users.first_name")
        if "last_name" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN last_name TEXT DEFAULT ''")
            print("  [Migration] Added Users.last_name")
        if "middle_name" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN middle_name TEXT DEFAULT ''")
            print("  [Migration] Added Users.middle_name")
        if "email" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN email TEXT DEFAULT ''")
            print("  [Migration] Added Users.email")
        if "phone" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN phone TEXT DEFAULT ''")
            print("  [Migration] Added Users.phone")
        if "cert_number" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN cert_number TEXT DEFAULT ''")
            print("  [Migration] Added Users.cert_number")
        # Ground-reality fields — Sprint 5 enterprise update
        if "company_id" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN company_id INTEGER REFERENCES Companies(company_id)")
            print("  [Migration] Added Users.company_id (Client role scoping)")
        if "subcontractor_company_name" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN subcontractor_company_name TEXT DEFAULT ''")
            print("  [Migration] Added Users.subcontractor_company_name (3rd party PDF legal name)")
        # Account approval system — Sprint 5 security hardening
        if "account_status" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN account_status TEXT DEFAULT 'active'")
            print("  [Migration] Added Users.account_status (pending/active/suspended/denied)")
        if "denial_reason" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN denial_reason TEXT DEFAULT ''")
            print("  [Migration] Added Users.denial_reason")
        if "applied_at" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN applied_at TEXT DEFAULT ''")
            print("  [Migration] Added Users.applied_at (signup timestamp)")

        # ── Assignments ────────────────────────────────────────────────────
        assign_cols = [r[1] for r in conn.execute("PRAGMA table_info(Assignments)").fetchall()]
        if "due_date" not in assign_cols:
            conn.execute("ALTER TABLE Assignments ADD COLUMN due_date DATE")
        if "notes" not in assign_cols:
            conn.execute("ALTER TABLE Assignments ADD COLUMN notes TEXT DEFAULT ''")
        if "org_id" not in assign_cols:
            conn.execute("ALTER TABLE Assignments ADD COLUMN org_id INTEGER REFERENCES Organizations(org_id)")

        # ── Reports ────────────────────────────────────────────────────────
        report_cols = [r[1] for r in conn.execute("PRAGMA table_info(Reports)").fetchall()]
        if "photo_path" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN photo_path TEXT")
        if "org_id" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN org_id INTEGER REFERENCES Organizations(org_id)")
        if "service_type" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN service_type TEXT DEFAULT 'Routine Inspection'")
        # NFPA 10 Section 7.2.2 — Monthly Visual Inspection Checklist
        nfpa_report_cols = {
            "chk_mounted":      "INTEGER DEFAULT 0",  # Mounted in designated location
            "chk_access":       "INTEGER DEFAULT 0",  # No obstruction to access/visibility
            "chk_pressure":     "INTEGER DEFAULT 0",  # Pressure gauge in operable range
            "chk_seal":         "INTEGER DEFAULT 0",  # Safety pin and tamper seal intact
            "chk_damage":       "INTEGER DEFAULT 0",  # No physical damage or corrosion
            "chk_nozzle":       "INTEGER DEFAULT 0",  # Nozzle/hose not blocked or damaged
            "chk_label":        "INTEGER DEFAULT 0",  # Operating instructions readable
            "chk_weight":       "INTEGER DEFAULT 0",  # Weight/heft correct (not discharged)
            "inspection_result":"TEXT DEFAULT ''",     # Pass, Fail, Needs Service
        }
        for col, col_type in nfpa_report_cols.items():
            if col not in report_cols:
                conn.execute(f"ALTER TABLE Reports ADD COLUMN {col} {col_type}")
                print(f"  [Migration] Added Reports.{col}")
        # Report legal immutability — Sprint 5
        if "approval_status" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN approval_status TEXT DEFAULT 'Approved'")
            print("  [Migration] Added Reports.approval_status")
        if "reviewed_by" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN reviewed_by INTEGER REFERENCES Users(user_id)")
            print("  [Migration] Added Reports.reviewed_by")
        if "reviewed_at" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN reviewed_at TEXT")
            print("  [Migration] Added Reports.reviewed_at")
        if "void_reason" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN void_reason TEXT DEFAULT ''")
            print("  [Migration] Added Reports.void_reason")
        if "po_number" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN po_number TEXT DEFAULT ''")
            print("  [Migration] Added Reports.po_number")

        # ── Companies table ────────────────────────────────────────────────
        if "Companies" not in tables:
            conn.execute("""
                CREATE TABLE Companies (
                    company_id   INTEGER PRIMARY KEY AUTOINCREMENT,
                    org_id       INTEGER REFERENCES Organizations(org_id),
                    name         TEXT NOT NULL,
                    address      TEXT,
                    city         TEXT,
                    state        TEXT,
                    zip          TEXT,
                    phone        TEXT,
                    contact_name TEXT,
                    created_at   TEXT DEFAULT (date('now'))
                )
            """)
        else:
            co_cols = [r[1] for r in conn.execute("PRAGMA table_info(Companies)").fetchall()]
            if "org_id" not in co_cols:
                conn.execute("ALTER TABLE Companies ADD COLUMN org_id INTEGER REFERENCES Organizations(org_id)")

        # ── Buildings table ────────────────────────────────────────────────
        if "Buildings" not in tables:
            conn.execute("""
                CREATE TABLE Buildings (
                    building_id  INTEGER PRIMARY KEY AUTOINCREMENT,
                    company_id   INTEGER NOT NULL REFERENCES Companies(company_id),
                    org_id       INTEGER REFERENCES Organizations(org_id),
                    name         TEXT NOT NULL,
                    address      TEXT,
                    floors       INTEGER DEFAULT 1,
                    notes        TEXT,
                    created_at   TEXT DEFAULT (date('now'))
                )
            """)
        else:
            bldg_cols = [r[1] for r in conn.execute("PRAGMA table_info(Buildings)").fetchall()]
            if "org_id" not in bldg_cols:
                conn.execute("ALTER TABLE Buildings ADD COLUMN org_id INTEGER REFERENCES Organizations(org_id)")

        # ── Add building_id + org_id to Extinguishers ─────────────────────
        ext_cols = [r[1] for r in conn.execute("PRAGMA table_info(Extinguishers)").fetchall()]
        if "building_id" not in ext_cols:
            conn.execute("ALTER TABLE Extinguishers ADD COLUMN building_id INTEGER REFERENCES Buildings(building_id)")
        if "org_id" not in ext_cols:
            conn.execute("ALTER TABLE Extinguishers ADD COLUMN org_id INTEGER REFERENCES Organizations(org_id)")

        # ── NFPA 10 compliance metadata on Extinguishers ──────────────────
        # Sprint 3 — adds classification, physical, and service tracking fields
        nfpa_cols = {
            "ext_type":            "TEXT DEFAULT ''",          # A, B, C, D, K, ABC, BC, etc.
            "ext_size_lbs":        "REAL",                     # capacity in pounds (e.g. 5, 10, 20)
            "serial_number":       "TEXT DEFAULT ''",          # manufacturer serial number
            "manufacturer":        "TEXT DEFAULT ''",          # brand name
            "model":               "TEXT DEFAULT ''",          # model number
            "manufacture_date":    "DATE",                     # for hydro test / 6-yr / 12-yr tracking
            "last_annual_date":    "DATE",                     # last NFPA annual maintenance
            "last_6year_date":     "DATE",                     # last 6-year internal exam
            "last_hydro_date":     "DATE",                     # last hydrostatic pressure test
            "ext_status":          "TEXT DEFAULT 'Active'",    # Active, Out of Service, Retired, Missing
            "last_inspection_date":"DATE",                     # last routine monthly inspection
            "dot_cert_no":         "TEXT DEFAULT ''",           # DOT certification number (e.g. A739)
            "condemned_date":      "DATE",                     # set when ext is condemned/removed
            "replaced_by_id":      "INTEGER",                  # FK to replacement extinguisher record
            "is_loaner":           "INTEGER DEFAULT 0",        # 1 = this unit is a loaner currently on-site
            "loaner_for_id":       "INTEGER",                  # FK to the original ext this unit is standing in for
        }
        for col, col_type in nfpa_cols.items():
            if col not in ext_cols:
                conn.execute(f"ALTER TABLE Extinguishers ADD COLUMN {col} {col_type}")
                print(f"  [Migration] Added Extinguishers.{col}")

        # ── AuditorTokens table — time-limited AHJ/Fire Marshal access ─────
        if "AuditorTokens" not in tables:
            conn.execute("""
                CREATE TABLE AuditorTokens (
                    token_id    INTEGER PRIMARY KEY AUTOINCREMENT,
                    token       TEXT NOT NULL UNIQUE,
                    org_id      INTEGER NOT NULL REFERENCES Organizations(org_id),
                    building_id INTEGER NOT NULL REFERENCES Buildings(building_id),
                    created_by  INTEGER REFERENCES Users(user_id),
                    created_at  TEXT DEFAULT (datetime('now')),
                    expires_at  TEXT NOT NULL,
                    label       TEXT DEFAULT ''
                )
            """)
            print("  [Migration] Created AuditorTokens table")

        # ── PlatformAnnouncements table ─────────────────────────────────────
        if "PlatformAnnouncements" not in tables:
            conn.execute("""
                CREATE TABLE PlatformAnnouncements (
                    announcement_id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title           TEXT NOT NULL,
                    body            TEXT NOT NULL,
                    priority        TEXT DEFAULT 'info',
                    created_at      TEXT DEFAULT (datetime('now')),
                    expires_at      TEXT,
                    is_active       INTEGER DEFAULT 1
                )
            """)
            conn.commit()
            print("  [Migration] Created PlatformAnnouncements table")

        # ── PasswordResetTokens table ───────────────────────────────────────
        if "PasswordResetTokens" not in tables:
            conn.execute("""
                CREATE TABLE PasswordResetTokens (
                    token_id   INTEGER PRIMARY KEY AUTOINCREMENT,
                    user_id    INTEGER NOT NULL REFERENCES Users(user_id),
                    token      TEXT NOT NULL UNIQUE,
                    expires_at TEXT NOT NULL,
                    used       INTEGER DEFAULT 0,
                    created_at TEXT DEFAULT (datetime('now'))
                )
            """)
            conn.commit()
            print("  [Migration] Created PasswordResetTokens table")

        # ── Messages table ──────────────────────────────────────────────────
        if "Messages" not in tables:
            conn.execute("""
                CREATE TABLE Messages (
                    message_id   INTEGER PRIMARY KEY AUTOINCREMENT,
                    org_id       INTEGER NOT NULL REFERENCES Organizations(org_id),
                    sender_id    INTEGER NOT NULL REFERENCES Users(user_id),
                    recipient_id INTEGER REFERENCES Users(user_id),
                    subject      TEXT NOT NULL DEFAULT '',
                    body         TEXT NOT NULL DEFAULT '',
                    is_read      INTEGER DEFAULT 0,
                    is_broadcast INTEGER DEFAULT 0,
                    broadcast_role TEXT DEFAULT '',
                    created_at   TEXT DEFAULT (datetime('now'))
                )
            """)
            conn.commit()
            print("  [Migration] Created Messages table")

        # ── ExtinguisherLocationHistory table ───────────────────────────────
        if "ExtinguisherLocationHistory" not in tables:
            conn.execute("""
                CREATE TABLE ExtinguisherLocationHistory (
                    history_id            INTEGER PRIMARY KEY AUTOINCREMENT,
                    extinguisher_id       INTEGER NOT NULL REFERENCES Extinguishers(extinguisher_id),
                    org_id                INTEGER NOT NULL REFERENCES Organizations(org_id),
                    changed_by_id         INTEGER REFERENCES Users(user_id),
                    changed_by_name       TEXT DEFAULT '',
                    changed_at            TEXT DEFAULT (datetime('now')),
                    old_building_id       INTEGER,
                    old_building_name     TEXT DEFAULT '',
                    old_floor_number      TEXT DEFAULT '',
                    old_room_number       TEXT DEFAULT '',
                    old_location_desc     TEXT DEFAULT '',
                    new_building_id       INTEGER,
                    new_building_name     TEXT DEFAULT '',
                    new_floor_number      TEXT DEFAULT '',
                    new_room_number       TEXT DEFAULT '',
                    new_location_desc     TEXT DEFAULT '',
                    notes                 TEXT DEFAULT ''
                )
            """)
            conn.commit()
            print("  [Migration] Created ExtinguisherLocationHistory table")

        # ── Organizations.last_active column ────────────────────────────────
        org_cols_v2 = [r[1] for r in conn.execute("PRAGMA table_info(Organizations)").fetchall()]
        if "last_active" not in org_cols_v2:
            conn.execute("ALTER TABLE Organizations ADD COLUMN last_active TEXT")
            conn.commit()
            print("  [Migration] Added Organizations.last_active")

        # ── NFPA interval defaults in PlatformSettings ──────────────────────
        existing_keys = {r[0] for r in conn.execute("SELECT key FROM PlatformSettings").fetchall()}
        for key, value in [
            ("nfpa_annual_interval_days", "365"),
            ("nfpa_6year_interval_days",  "2190"),
            ("nfpa_hydro_interval_days",  "3650"),
        ]:
            if key not in existing_keys:
                conn.execute("INSERT INTO PlatformSettings (key, value) VALUES (?,?)", (key, value))
        conn.commit()

        conn.commit()

        # ── Auto-migrate: backfill last_report_id and last_inspection_date ──
        # For extinguishers that have reports but null last_report_id
        conn.execute("""
            UPDATE Extinguishers SET
                last_report_id = (SELECT r.report_id FROM Reports r
                    WHERE r.extinguisher_id = Extinguishers.extinguisher_id
                    ORDER BY r.inspection_date DESC, r.report_id DESC LIMIT 1),
                last_inspection_date = (SELECT r.inspection_date FROM Reports r
                    WHERE r.extinguisher_id = Extinguishers.extinguisher_id
                    ORDER BY r.inspection_date DESC, r.report_id DESC LIMIT 1)
            WHERE last_report_id IS NULL
              AND extinguisher_id IN (SELECT DISTINCT extinguisher_id FROM Reports)
        """)
        conn.commit()

        # ── Auto-migrate: seed default org for existing data ──────────────
        # Only runs once — when Organizations table is empty but Users exist
        org_count  = conn.execute("SELECT COUNT(*) FROM Organizations").fetchone()[0]
        user_count = conn.execute("SELECT COUNT(*) FROM Users").fetchone()[0]

        if org_count == 0 and user_count > 0:
            # Create a default org for all existing data
            conn.execute("""
                INSERT INTO Organizations (name, address, city, state)
                VALUES ('Fire Wardens Safety', 'National University', 'San Diego', 'CA')
            """)
            conn.commit()
            default_org = conn.execute("SELECT last_insert_rowid()").fetchone()[0]

            # Stamp every existing table row with the default org
            conn.execute("UPDATE Users          SET org_id=? WHERE org_id IS NULL", (default_org,))
            conn.execute("UPDATE Companies      SET org_id=? WHERE org_id IS NULL", (default_org,))
            conn.execute("UPDATE Buildings      SET org_id=? WHERE org_id IS NULL", (default_org,))
            conn.execute("UPDATE Extinguishers  SET org_id=? WHERE org_id IS NULL", (default_org,))
            conn.execute("UPDATE Assignments    SET org_id=? WHERE org_id IS NULL", (default_org,))
            conn.execute("UPDATE Reports        SET org_id=? WHERE org_id IS NULL", (default_org,))
            conn.commit()
            print(f"  [Migration] Created default org #{default_org} and stamped all existing data.")

        # ── Auto-migrate extinguisher data into Companies/Buildings ────────
        co_count  = conn.execute("SELECT COUNT(*) FROM Companies WHERE org_id IS NOT NULL").fetchone()[0]
        ext_count = conn.execute("SELECT COUNT(*) FROM Extinguishers").fetchone()[0]

        if co_count == 0 and ext_count > 0:
            default_org = conn.execute("SELECT org_id FROM Organizations LIMIT 1").fetchone()[0]
            rows = conn.execute(
                "SELECT extinguisher_id, address, building_number FROM Extinguishers"
            ).fetchall()
            address_to_company = {}
            for row in rows:
                addr = row["address"] or "Unknown Address"
                if addr not in address_to_company:
                    conn.execute(
                        "INSERT INTO Companies (org_id, name, address) VALUES (?, ?, ?)",
                        (default_org, addr, addr)
                    )
                    conn.commit()
                    address_to_company[addr] = conn.execute("SELECT last_insert_rowid()").fetchone()[0]

            addr_bldg_to_building = {}
            for row in rows:
                addr     = row["address"] or "Unknown Address"
                bldg_num = row["building_number"] or "Main Building"
                key      = (addr, bldg_num)
                if key not in addr_bldg_to_building:
                    co_id = address_to_company[addr]
                    conn.execute(
                        "INSERT INTO Buildings (org_id, company_id, name, address) VALUES (?,?,?,?)",
                        (default_org, co_id, bldg_num, addr)
                    )
                    conn.commit()
                    addr_bldg_to_building[key] = conn.execute("SELECT last_insert_rowid()").fetchone()[0]

            for row in rows:
                addr     = row["address"] or "Unknown Address"
                bldg_num = row["building_number"] or "Main Building"
                bldg_id  = addr_bldg_to_building[(addr, bldg_num)]
                conn.execute(
                    "UPDATE Extinguishers SET building_id=?, org_id=? WHERE extinguisher_id=?",
                    (bldg_id, default_org, row["extinguisher_id"])
                )
            conn.commit()

        # ── ServiceRequests table ──────────────────────────────────────────
        if "ServiceRequests" not in tables:
            conn.execute("""
                CREATE TABLE ServiceRequests (
                    request_id      INTEGER PRIMARY KEY AUTOINCREMENT,
                    org_id          INTEGER REFERENCES Organizations(org_id),
                    company_id      INTEGER REFERENCES Companies(company_id),
                    extinguisher_id INTEGER REFERENCES Extinguishers(extinguisher_id),
                    requested_by    TEXT NOT NULL,
                    request_type    TEXT NOT NULL DEFAULT 'Service/Repair',
                    part_number     TEXT DEFAULT '',
                    quantity        INTEGER DEFAULT 1,
                    vendor          TEXT DEFAULT '',
                    estimated_cost  REAL DEFAULT 0.0,
                    notes           TEXT DEFAULT '',
                    status          TEXT DEFAULT 'Pending',
                    admin_notes     TEXT DEFAULT '',
                    created_at      TEXT DEFAULT (datetime('now')),
                    updated_at      TEXT DEFAULT (datetime('now'))
                )
            """)
            conn.commit()
            print("  [Migration] Created ServiceRequests table")

run_migrations()

# ── Serve frontend ─────────────────────────────────────────────────────────────
@app.route('/')
def landing():
    return send_from_directory(BASE_DIR, 'landing.html')

@app.route('/app')
def index():
    return send_from_directory(UI_DIR, 'index.html')

@app.route("/<path:filename>")
def static_files(filename):
    return send_from_directory(UI_DIR, filename)

@app.route("/uploads/<path:filename>")
def serve_upload(filename):
    return send_from_directory(UPLOAD_DIR, filename)

@app.route("/backgrounds/<path:filename>")
def serve_background(filename):
    """Serve platform background images."""
    return send_from_directory(BACKGROUND_DIR, filename)

# ── PHOTO UPLOAD ───────────────────────────────────────────────────────────────
@app.route("/api/upload", methods=["POST"])
def upload_photo():
    """Handle inspection photo uploads.
    Renames file with UUID to prevent collisions and path traversal.
    Validates file extension against ALLOWED_EXTENSIONS.
    Returns the UUID filename for storage in Reports.photo_path."""
    if "photo" not in request.files:
        return jsonify({"error": "No photo file provided"}), 400
    file = request.files["photo"]
    if file.filename == "":
        return jsonify({"error": "Empty filename"}), 400
    ext = file.filename.rsplit(".", 1)[-1].lower() if "." in file.filename else ""
    if ext not in ALLOWED_EXTENSIONS:
        return jsonify({"error": f"File type .{ext} not allowed"}), 400
    # Generate unique filename to avoid collisions
    unique_name = f"{uuid.uuid4().hex}.{ext}"
    file.save(os.path.join(UPLOAD_DIR, unique_name))
    return jsonify({"success": True, "filename": unique_name}), 201
# ── ORGANIZATION REGISTRATION ──────────────────────────────────────────────────
@app.route("/api/register", methods=["POST"])
def register_org():
    """Register a new organization with its first admin account.
    Creates the Organization row, then creates the admin User.
    If platform approval is required (PlatformSettings.require_approval),
    the new org starts as "pending" and needs Super Admin approval."""
    d        = request.get_json()
    org_name = (d.get("org_name")  or "").strip()
    username = (d.get("username")  or "").strip()
    password = (d.get("password")  or "").strip()
    address  = (d.get("address")   or "").strip()
    city     = (d.get("city")      or "").strip()
    state    = (d.get("state")     or "").strip()
    phone    = (d.get("phone")     or "").strip()
    contact  = (d.get("contact_name") or "").strip()

    if not org_name:
        return jsonify({"error": "Organization name is required."}), 400
    if not username or not password:
        return jsonify({"error": "Admin username and password are required."}), 400
    if len(password) < 64:
        return jsonify({"error": "Password must be SHA-256 hashed before sending."}), 400

    try:
        with get_db() as conn:
            # Check org name not already taken
            if conn.execute("SELECT org_id FROM Organizations WHERE name=?", (org_name,)).fetchone():
                return jsonify({"error": "An organization with that name already exists."}), 409
            # Check username not already taken globally
            if conn.execute("SELECT user_id FROM Users WHERE username=?", (username,)).fetchone():
                return jsonify({"error": "That username is already taken."}), 409

            # Create org — check if approval is required
            approval_row = conn.execute(
                "SELECT value FROM PlatformSettings WHERE key='require_approval'"
            ).fetchone()
            require_approval = (approval_row and approval_row["value"] == "true") if approval_row else True
            initial_status = "pending" if require_approval else "approved"

            conn.execute(
                "INSERT INTO Organizations (name, address, city, state, phone, contact_name, platform_status) "
                "VALUES (?,?,?,?,?,?,?)",
                (org_name, address, city, state, phone, contact, initial_status)
            )
            conn.commit()
            org_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]

            # Create first Admin account for this org
            conn.execute(
                "INSERT INTO Users (username, password_hash, role, org_id) VALUES (?,?,?,?)",
                (username, password, "Admin", org_id)
            )
            conn.commit()
            user_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]

        platform_log("system", "org_registered", org_name, f"New org registered by {username} (status: {initial_status})")
        return jsonify({
            "success":  True,
            "org_id":   org_id,
            "org_name": org_name,
            "user_id":  user_id,
            "username": username,
            "role":     "Admin",
            "platform_status": initial_status
        }), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/signup", methods=["POST"])
def signup():
    """Self-registration for new users.
    Creates account with status=pending_approval for Admin review.
    The user sets their own password — Admin never handles it.
    Requires a valid org_id so the account is tied to the correct organization."""
    data = request.get_json()
    required = ["username", "password", "role", "org_id", "first_name", "last_name", "email"]
    for field in required:
        if not data.get(field, "").strip() if isinstance(data.get(field), str) else not data.get(field):
            return jsonify({"success": False, "error": f"Missing required field: {field}"}), 400

    username   = data["username"].strip()
    password   = data["password"]   # Already SHA-256 hashed by client
    role       = data["role"].strip()
    org_id     = data["org_id"]
    first_name = data["first_name"].strip()
    last_name  = data["last_name"].strip()
    middle_name = data.get("middle_name", "").strip()
    email      = data["email"].strip()
    phone      = data.get("phone", "").strip()
    cert_number = data.get("cert_number", "").strip()
    subcontractor_company_name = data.get("subcontractor_company_name", "").strip()

    # Validate role — Admin and Platform Admin cannot self-register
    allowed_roles = ["Inspector", "3rd_Party_Inspector", "3rd_Party_Admin", "Client"]
    if role not in allowed_roles:
        return jsonify({"success": False, "error": "Invalid role. Admins are created by the organization."}), 400

    with get_db() as conn:
        # Verify org exists
        org = conn.execute(
            "SELECT org_id, name FROM Organizations WHERE org_id=?", (org_id,)
        ).fetchone()
        if not org:
            return jsonify({"success": False, "error": "Organization ID not found. Please check your Organization ID and try again."}), 404

        # Check username not already taken
        existing = conn.execute(
            "SELECT user_id FROM Users WHERE username=?", (username,)
        ).fetchone()
        if existing:
            return jsonify({"success": False, "error": "Username already taken. Please choose a different username."}), 409

        # Check email not already registered in this org
        existing_email = conn.execute(
            "SELECT user_id FROM Users WHERE email=? AND org_id=?", (email, org_id)
        ).fetchone()
        if existing_email:
            return jsonify({"success": False, "error": "An account with this email already exists in this organization."}), 409

        import datetime
        applied_at = datetime.datetime.utcnow().isoformat()

        conn.execute("""
            INSERT INTO Users
              (username, password_hash, role, org_id,
               first_name, last_name, middle_name, email, phone,
               cert_number, subcontractor_company_name,
               account_status, applied_at)
            VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)
        """, (
            username, password, role, org_id,
            first_name, last_name, middle_name, email, phone,
            cert_number, subcontractor_company_name,
            "pending", applied_at
        ))
        conn.commit()

        platform_log(username, "user_signup", org["name"],
            f"role={role}, status=pending, email={email}")

    return jsonify({
        "success": True,
        "message": f"Account created successfully. Your request has been sent to the {org['name']} administrator for approval. You will be able to log in once approved."
    }), 201


@app.route("/api/login", methods=["POST"])
def login():
    """Authenticate a user by comparing SHA-256 hash of password.
    Returns user info, org details, and platform_status (for pending org overlay).
    The client hashes the password before sending, so we compare hashes directly."""
    data     = request.get_json()
    username = (data.get("username") or "").strip()
    password = data.get("password") or ""

    if not username or not password:
        return jsonify({"success": False, "error": "Missing credentials"}), 400

    with get_db() as conn:
        row = conn.execute(
            """SELECT user_id, username, role, org_id, company_id,
                      first_name, last_name, middle_name, email, phone, cert_number,
                      subcontractor_company_name, account_status
               FROM Users
               WHERE username = ? AND password_hash = ?""",
            (username, password)
        ).fetchone()

    if row:
        # Block login based on individual account status BEFORE checking org status
        acct_status = row["account_status"] or "active"
        if acct_status == "pending":
            return jsonify({
                "success": False,
                "error": "Your account is pending approval by your organization administrator. You will be notified once approved."
            }), 403
        if acct_status == "suspended":
            return jsonify({
                "success": False,
                "error": "Your account has been suspended. Contact your administrator."
            }), 403
        if acct_status == "denied":
            return jsonify({
                "success": False,
                "error": "Your account registration was not approved. You may re-apply or contact your administrator."
            }), 403

        # Build the legal full name for NFPA forms
        full_name_parts = [row["first_name"], row["middle_name"], row["last_name"]]
        full_name = " ".join(p for p in full_name_parts if p).strip() or row["username"]

        # Get org name and platform status for display
        org_name = ""
        platform_status = "approved"
        if row["org_id"]:
            org = get_db().execute(
                "SELECT name, platform_status FROM Organizations WHERE org_id=?", (row["org_id"],)
            ).fetchone()
            if org:
                org_name = org["name"]
                platform_status = org["platform_status"] or "approved"
        platform_log(row["username"], "user_login", org_name, f"role={row['role']}")
        # Update org's last_active timestamp on every successful login
        if row["org_id"]:
            with get_db() as conn:
                conn.execute(
                    "UPDATE Organizations SET last_active=datetime('now') WHERE org_id=?",
                    (row["org_id"],)
                )
        # Block login for suspended or rejected orgs
        if platform_status in ("suspended", "rejected"):
            platform_log(row["username"], "login_blocked", org_name, f"Org status: {platform_status}")
            return jsonify({
                "success": True,
                "user_id":  row["user_id"],
                "username": row["username"],
                "role":     row["role"],
                "org_id":   row["org_id"],
                "org_name": org_name,
                "platform_status": platform_status,
            })
        return jsonify({
            "success":         True,
            "user_id":         row["user_id"],
            "username":        row["username"],
            "role":            row["role"],
            "org_id":          row["org_id"],
            "company_id":      row["company_id"],  # Client role scoping
            "org_name":        org_name,
            "platform_status": platform_status,
            # PII fields — used for NFPA form Section 6 (Inspector Certification)
            "full_name":    full_name,
            "first_name":   row["first_name"]  or "",
            "last_name":    row["last_name"]   or "",
            "email":        row["email"]        or "",
            "phone":        row["phone"]        or "",
            "cert_number":  row["cert_number"]  or "",
            "subcontractor_company_name": row["subcontractor_company_name"] or "",
        })
    platform_log("unknown", "login_failed", username, "Invalid credentials")
    return jsonify({"success": False, "error": "Invalid username or password"}), 401


# ── COMPANIES ──────────────────────────────────────────────────────────────────
@app.route("/api/users/<int:user_id>/status", methods=["POST"])
def update_user_status(user_id):
    """Admin: approve, deny, or suspend a user account.
    - approve: sets account_status='active', user can now log in
    - deny:    sets account_status='denied', user can re-apply (re-registers)
    - suspend: sets account_status='suspended', user sees specific message on login
    Requires caller to be Admin role (validated via org_id scope)."""
    data    = request.get_json()
    action  = (data.get("action") or "").lower()   # approve / deny / suspend / reactivate
    reason  = data.get("reason", "").strip()
    org_id  = data.get("org_id")

    if action not in ("approve", "deny", "suspend", "reactivate"):
        return jsonify({"success": False, "error": "Invalid action"}), 400

    status_map = {
        "approve":    "active",
        "deny":       "denied",
        "suspend":    "suspended",
        "reactivate": "active",
    }
    new_status = status_map[action]

    with get_db() as conn:
        # Scope to org — Admin cannot affect users in other orgs
        row = conn.execute(
            "SELECT user_id, username, org_id FROM Users WHERE user_id=? AND org_id=?",
            (user_id, org_id)
        ).fetchone()
        if not row:
            return jsonify({"success": False, "error": "User not found"}), 404

        conn.execute(
            "UPDATE Users SET account_status=?, denial_reason=? WHERE user_id=?",
            (new_status, reason, user_id)
        )
        conn.commit()
        platform_log("admin", f"user_{action}", str(org_id),
            f"user_id={user_id}, username={row['username']}, new_status={new_status}")

    return jsonify({"success": True, "status": new_status})


@app.route("/api/users/pending", methods=["GET"])
def get_pending_users():
    """Admin: get all users with account_status='pending' for this org.
    Used to populate the approval queue in the Admin dashboard."""
    org_id = request.args.get("org_id")
    if not org_id:
        return jsonify([])
    with get_db() as conn:
        rows = conn.execute("""
            SELECT user_id, username, role, first_name, last_name, email,
                   phone, cert_number, subcontractor_company_name,
                   account_status, denial_reason, applied_at
            FROM Users
            WHERE org_id=? AND account_status IN ('pending','denied','suspended')
            ORDER BY applied_at DESC
        """, (org_id,)).fetchall()
    return jsonify([dict(r) for r in rows])


@app.route("/api/companies", methods=["GET"])
def get_companies():
    """List all companies for an organization, with building and extinguisher counts.
    Used by the drill-down hierarchy (Company → Building → Extinguisher).
    Supports optional company_id filter for Client role scoping."""
    org_id     = request.args.get("org_id")
    company_id = request.args.get("company_id")  # Client: scope to their company only
    with get_db() as conn:
        if company_id:
            rows = conn.execute("""
                SELECT c.company_id, c.org_id, c.name, c.address, c.city, c.state, c.phone, c.contact_name,
                       o.name AS org_name,
                       COUNT(DISTINCT b.building_id) AS building_count,
                       COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count
                FROM Companies c
                JOIN Organizations o ON o.org_id = c.org_id
                LEFT JOIN Buildings b ON b.company_id = c.company_id
                LEFT JOIN Extinguishers e ON e.building_id = b.building_id
                WHERE c.org_id = ? AND c.company_id = ?
                GROUP BY c.company_id
                ORDER BY c.name
            """, (org_id, company_id)).fetchall()
        elif org_id:
            rows = conn.execute("""
                SELECT c.company_id, c.org_id, c.name, c.address, c.city, c.state, c.phone, c.contact_name,
                       o.name AS org_name,
                       COUNT(DISTINCT b.building_id) AS building_count,
                       COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count
                FROM Companies c
                JOIN Organizations o ON o.org_id = c.org_id
                LEFT JOIN Buildings b ON b.company_id = c.company_id
                LEFT JOIN Extinguishers e ON e.building_id = b.building_id
                WHERE c.org_id = ?
                GROUP BY c.company_id
                ORDER BY c.name
            """, (org_id,)).fetchall()
        else:
            # Platform-wide: all companies across all orgs (platform admin use)
            rows = conn.execute("""
                SELECT c.company_id, c.org_id, c.name, c.address, c.city, c.state, c.phone, c.contact_name,
                       o.name AS org_name,
                       COUNT(DISTINCT b.building_id) AS building_count,
                       COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count
                FROM Companies c
                JOIN Organizations o ON o.org_id = c.org_id
                LEFT JOIN Buildings b ON b.company_id = c.company_id
                LEFT JOIN Extinguishers e ON e.building_id = b.building_id
                GROUP BY c.company_id
                ORDER BY o.name, c.name
            """).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/companies", methods=["POST"])
def add_company():
    d = request.get_json()
    name   = (d.get("name")   or "").strip()
    org_id = d.get("org_id")
    if not name:
        return jsonify({"error": "Company name is required."}), 400
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Companies (org_id, name, address, city, state, zip, phone, contact_name) "
                "VALUES (?,?,?,?,?,?,?,?)",
                (org_id, name, d.get("address",""), d.get("city",""), d.get("state",""),
                 d.get("zip",""), d.get("phone",""), d.get("contact_name",""))
            )
        platform_log("admin", "company_created", name, f"org_id={org_id}")
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/companies/<int:company_id>", methods=["PUT"])
def update_company(company_id):
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "UPDATE Companies SET name=?, address=?, city=?, state=?, zip=?, phone=?, contact_name=? "
                "WHERE company_id=?",
                (d.get("name"), d.get("address",""), d.get("city",""), d.get("state",""),
                 d.get("zip",""), d.get("phone",""), d.get("contact_name",""), company_id)
            )
        platform_log("admin", "company_updated", str(company_id), "Updated")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/companies/<int:company_id>", methods=["DELETE"])
def delete_company(company_id):
    """Delete a company and cascade to all its buildings, extinguishers,
    reports, and assignments. This is a destructive operation."""
    try:
        with get_db() as conn:
            bldgs = conn.execute(
                "SELECT building_id FROM Buildings WHERE company_id=?", (company_id,)
            ).fetchall()
            for b in bldgs:
                bid = b["building_id"]
                exts = conn.execute(
                    "SELECT extinguisher_id FROM Extinguishers WHERE building_id=?", (bid,)
                ).fetchall()
                for e in exts:
                    eid = e["extinguisher_id"]
                    conn.execute("DELETE FROM Reports     WHERE extinguisher_id=?", (eid,))
                    conn.execute("DELETE FROM Assignments WHERE extinguisher_id=?", (eid,))
                conn.execute("DELETE FROM Extinguishers WHERE building_id=?", (bid,))
            conn.execute("DELETE FROM Buildings WHERE company_id=?", (company_id,))
            conn.execute("DELETE FROM Companies WHERE company_id=?", (company_id,))
        platform_log("admin", "company_deleted", str(company_id), "Cascade delete")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── BUILDINGS ──────────────────────────────────────────────────────────────────
@app.route("/api/buildings", methods=["GET"])
def get_buildings():
    company_id = request.args.get("company_id")
    org_id     = request.args.get("org_id")
    with get_db() as conn:
        if company_id:
            rows = conn.execute("""
                SELECT b.building_id, b.company_id, b.name, b.address, b.floors, b.notes,
                       c.name AS company_name,
                       COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count,
                       SUM(CASE WHEN e.next_due_date < date('now') AND e.next_due_date IS NOT NULL THEN 1 ELSE 0 END) AS overdue_count,
                       SUM(CASE WHEN a.status = 'Pending Inspection' THEN 1 ELSE 0 END) AS pending_count
                FROM Buildings b
                JOIN Companies c ON c.company_id = b.company_id
                LEFT JOIN Extinguishers e ON e.building_id = b.building_id
                LEFT JOIN Assignments a ON a.extinguisher_id = e.extinguisher_id AND a.status = 'Pending Inspection'
                WHERE b.company_id = ?
                GROUP BY b.building_id ORDER BY b.name
            """, (company_id,)).fetchall()
        elif org_id:
            rows = conn.execute("""
                SELECT b.building_id, b.company_id, b.name, b.address, b.floors, b.notes,
                       c.name AS company_name, o.name AS org_name,
                       COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count,
                       SUM(CASE WHEN e.next_due_date < date('now') AND e.next_due_date IS NOT NULL THEN 1 ELSE 0 END) AS overdue_count,
                       SUM(CASE WHEN a.status = 'Pending Inspection' THEN 1 ELSE 0 END) AS pending_count
                FROM Buildings b
                JOIN Companies c ON c.company_id = b.company_id
                JOIN Organizations o ON o.org_id = b.org_id
                LEFT JOIN Extinguishers e ON e.building_id = b.building_id
                LEFT JOIN Assignments a ON a.extinguisher_id = e.extinguisher_id AND a.status = 'Pending Inspection'
                WHERE b.org_id = ?
                GROUP BY b.building_id ORDER BY c.name, b.name
            """, (org_id,)).fetchall()
        else:
            # Platform-wide: all buildings across all orgs (platform admin use)
            rows = conn.execute("""
                SELECT b.building_id, b.company_id, b.name, b.address, b.floors, b.notes,
                       c.name AS company_name, o.name AS org_name,
                       COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count,
                       SUM(CASE WHEN e.next_due_date < date('now') AND e.next_due_date IS NOT NULL THEN 1 ELSE 0 END) AS overdue_count,
                       SUM(CASE WHEN a.status = 'Pending Inspection' THEN 1 ELSE 0 END) AS pending_count
                FROM Buildings b
                JOIN Companies c ON c.company_id = b.company_id
                JOIN Organizations o ON o.org_id = b.org_id
                LEFT JOIN Extinguishers e ON e.building_id = b.building_id
                LEFT JOIN Assignments a ON a.extinguisher_id = e.extinguisher_id AND a.status = 'Pending Inspection'
                GROUP BY b.building_id ORDER BY o.name, c.name, b.name
            """).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/buildings", methods=["POST"])
def add_building():
    d          = request.get_json()
    name       = (d.get("name") or "").strip()
    company_id = d.get("company_id")
    org_id     = d.get("org_id")
    if not name or not company_id:
        return jsonify({"error": "Building name and company_id are required."}), 400
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Buildings (org_id, company_id, name, address, floors, notes) VALUES (?,?,?,?,?,?)",
                (org_id, company_id, name, d.get("address",""), d.get("floors",1), d.get("notes",""))
            )
        platform_log("admin", "building_created", name, f"org_id={d.get('org_id')}")
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/buildings/<int:building_id>", methods=["PUT"])
def update_building(building_id):
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "UPDATE Buildings SET name=?, address=?, floors=?, notes=? WHERE building_id=?",
                (d.get("name"), d.get("address",""), d.get("floors",1), d.get("notes",""), building_id)
            )
        platform_log("admin", "building_updated", str(building_id), "Updated")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/buildings/<int:building_id>", methods=["DELETE"])
def delete_building(building_id):
    try:
        with get_db() as conn:
            exts = conn.execute(
                "SELECT extinguisher_id FROM Extinguishers WHERE building_id=?", (building_id,)
            ).fetchall()
            for e in exts:
                eid = e["extinguisher_id"]
                conn.execute("DELETE FROM Reports     WHERE extinguisher_id=?", (eid,))
                conn.execute("DELETE FROM Assignments WHERE extinguisher_id=?", (eid,))
            conn.execute("DELETE FROM Extinguishers WHERE building_id=?", (building_id,))
            conn.execute("DELETE FROM Buildings WHERE building_id=?", (building_id,))
        platform_log("admin", "building_deleted", str(building_id), "Cascade delete")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── EXTINGUISHERS ──────────────────────────────────────────────────────────────
@app.route("/api/extinguishers", methods=["GET"])
def get_extinguishers():
    """List extinguishers with JOINs to Buildings and Companies for location context.
    Supports filtering by building_id, company_id, or org_id.
    Returns all columns including NFPA 10 compliance metadata.
    Client role: automatically scoped to their company_id only.
    Inspector / 3rd Party Inspector: scoped to buildings in their assigned work orders."""
    building_id = request.args.get("building_id")
    company_id  = request.args.get("company_id")
    org_id      = request.args.get("org_id")
    # API-level Client scoping — enforce even if someone calls the API directly
    client_company_id = request.args.get("client_company_id")

    with get_db() as conn:
        base = """
            SELECT e.*,
                   b.name    AS building_name,
                   c.name    AS company_name,
                   c.company_id AS company_id_fk
            FROM Extinguishers e
            LEFT JOIN Buildings b ON b.building_id = e.building_id
            LEFT JOIN Companies c ON c.company_id  = b.company_id
        """
        if client_company_id:
            # Client role — strictly scoped to their company only
            rows = conn.execute(
                base + " WHERE c.company_id=? ORDER BY b.name, e.floor_number, e.room_number",
                (client_company_id,)
            ).fetchall()
        elif building_id:
            rows = conn.execute(base + " WHERE e.building_id=? ORDER BY e.floor_number, e.room_number",
                                (building_id,)).fetchall()
        elif company_id:
            rows = conn.execute(base + " WHERE c.company_id=? ORDER BY b.name, e.floor_number, e.room_number",
                                (company_id,)).fetchall()
        elif org_id:
            rows = conn.execute(base + " WHERE e.org_id=? ORDER BY c.name, b.name, e.floor_number, e.room_number",
                                (org_id,)).fetchall()
        else:
            rows = conn.execute(base + " ORDER BY c.name, b.name, e.floor_number, e.room_number").fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/extinguishers", methods=["POST"])
def add_extinguisher():
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Extinguishers "
                "(address, building_number, floor_number, room_number, "
                " location_description, inspection_interval_days, next_due_date, building_id, org_id,"
                " ext_type, ext_size_lbs, serial_number, manufacturer, model,"
                " manufacture_date, last_annual_date, last_6year_date, last_hydro_date, ext_status, dot_cert_no) "
                "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (d.get("address"), d.get("building_number"),
                 d.get("floor_number", 1), d.get("room_number"),
                 d.get("location_description"),
                 d.get("inspection_interval_days", 30),
                 d.get("next_due_date"),
                 d.get("building_id"),
                 d.get("org_id"),
                 d.get("ext_type", ""),
                 d.get("ext_size_lbs"),
                 d.get("serial_number", ""),
                 d.get("manufacturer", ""),
                 d.get("model", ""),
                 d.get("manufacture_date"),
                 d.get("last_annual_date"),
                 d.get("last_6year_date"),
                 d.get("last_hydro_date"),
                 d.get("ext_status", "Active"),
                 d.get("dot_cert_no", ""))
            )
        platform_log("admin", "extinguisher_created", "new", f"org_id={d.get('org_id')}")
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/extinguishers/<int:ext_id>", methods=["GET"])
def get_extinguisher_by_id(ext_id):
    """Return a single extinguisher row by ID with full building/company context.
    Used by the QR code deep-link feature: when a user scans a QR code,
    the frontend fetches this endpoint to get the full row for openExtDetail()."""
    with get_db() as conn:
        row = conn.execute("""
            SELECT e.*,
                   b.name    AS building_name,
                   c.name    AS company_name,
                   c.company_id AS company_id_fk
            FROM Extinguishers e
            LEFT JOIN Buildings b ON b.building_id = e.building_id
            LEFT JOIN Companies c ON c.company_id  = b.company_id
            WHERE e.extinguisher_id = ?
        """, (ext_id,)).fetchone()
    if not row:
        return jsonify({"error": "Extinguisher not found"}), 404
    return jsonify(dict(row))

@app.route("/api/extinguishers/<int:ext_id>", methods=["PUT"])
def update_extinguisher(ext_id):
    d = request.get_json()
    try:
        with get_db() as conn:
            # ── Snapshot the current location fields before overwriting ──────
            old = conn.execute(
                "SELECT building_id, floor_number, room_number, location_description "
                "FROM Extinguishers WHERE extinguisher_id=?", (ext_id,)
            ).fetchone()

            conn.execute(
                "UPDATE Extinguishers SET address=?, building_number=?, "
                "floor_number=?, room_number=?, location_description=?, "
                "inspection_interval_days=?, next_due_date=?, building_id=?,"
                "ext_type=?, ext_size_lbs=?, serial_number=?, manufacturer=?, model=?,"
                "manufacture_date=?, last_annual_date=?, last_6year_date=?, last_hydro_date=?, ext_status=?, dot_cert_no=? "
                "WHERE extinguisher_id=?",
                (d.get("address"), d.get("building_number"),
                 d.get("floor_number", 1), d.get("room_number"),
                 d.get("location_description"),
                 d.get("inspection_interval_days", 30),
                 d.get("next_due_date"),
                 d.get("building_id"),
                 d.get("ext_type", ""),
                 d.get("ext_size_lbs"),
                 d.get("serial_number", ""),
                 d.get("manufacturer", ""),
                 d.get("model", ""),
                 d.get("manufacture_date"),
                 d.get("last_annual_date"),
                 d.get("last_6year_date"),
                 d.get("last_hydro_date"),
                 d.get("ext_status", "Active"),
                 d.get("dot_cert_no", ""),
                 ext_id)
            )

            # ── Log a location-history row if any location field changed ─────
            new_building_id = d.get("building_id")
            new_floor       = str(d.get("floor_number", "")).strip()
            new_room        = str(d.get("room_number") or "").strip()
            new_loc_desc    = str(d.get("location_description") or "").strip()

            old_building_id = old["building_id"] if old else None
            old_floor       = str(old["floor_number"] or "").strip() if old else ""
            old_room        = str(old["room_number"] or "").strip() if old else ""
            old_loc_desc    = str(old["location_description"] or "").strip() if old else ""

            location_changed = (
                str(new_building_id) != str(old_building_id) or
                new_floor  != old_floor  or
                new_room   != old_room   or
                new_loc_desc != old_loc_desc
            )

            if old and location_changed:
                # Resolve building names for both old and new building IDs
                def _bname(bid):
                    if not bid:
                        return ""
                    row = conn.execute(
                        "SELECT name FROM Buildings WHERE building_id=?", (bid,)
                    ).fetchone()
                    return row["name"] if row else str(bid)

                org_id       = d.get("org_id")
                changed_by   = d.get("user_id")
                changer_name = ""
                if changed_by:
                    u = conn.execute(
                        "SELECT first_name, last_name FROM Users WHERE user_id=?",
                        (changed_by,)
                    ).fetchone()
                    if u:
                        changer_name = f"{u['first_name']} {u['last_name']}".strip()

                conn.execute("""
                    INSERT INTO ExtinguisherLocationHistory
                        (extinguisher_id, org_id, changed_by_id, changed_by_name,
                         old_building_id, old_building_name, old_floor_number,
                         old_room_number, old_location_desc,
                         new_building_id, new_building_name, new_floor_number,
                         new_room_number, new_location_desc)
                    VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)
                """, (
                    ext_id, org_id, changed_by, changer_name,
                    old_building_id, _bname(old_building_id),
                    old_floor, old_room, old_loc_desc,
                    new_building_id, _bname(new_building_id),
                    new_floor, new_room, new_loc_desc
                ))

        platform_log("admin", "extinguisher_updated", str(ext_id), "Updated")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/extinguishers/<int:ext_id>", methods=["DELETE"])
def delete_extinguisher(ext_id):
    try:
        with get_db() as conn:
            conn.execute("DELETE FROM Reports     WHERE extinguisher_id=?", (ext_id,))
            conn.execute("DELETE FROM Assignments WHERE extinguisher_id=?", (ext_id,))
            conn.execute("DELETE FROM Extinguishers WHERE extinguisher_id=?", (ext_id,))
        platform_log("admin", "extinguisher_deleted", str(ext_id), "Cascade delete")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── ASSIGNMENTS ────────────────────────────────────────────────────────────────
@app.route("/api/assignments", methods=["GET"])
def get_assignments():
    """List assignments with full context (admin name, inspector name, location).
    JOINs through Users (admin + inspector), Extinguishers, Buildings, Companies.
    Filterable by org_id and optionally user_id (for inspector-scoped views)."""
    user_id = request.args.get("user_id")
    org_id  = request.args.get("org_id")
    with get_db() as conn:
        existing = [r[1] for r in conn.execute("PRAGMA table_info(Assignments)").fetchall()]
        if "due_date" not in existing:
            conn.execute("ALTER TABLE Assignments ADD COLUMN due_date DATE")
        if "notes" not in existing:
            conn.execute("ALTER TABLE Assignments ADD COLUMN notes TEXT DEFAULT ''")

        base = """
            SELECT a.assignment_id, a.inspector_id, a.extinguisher_id,
                   a.due_date, a.status, a.notes,
                   u_a.username AS assigned_by,
                   u_i.username AS inspector,
                   e.floor_number, e.room_number, e.location_description,
                   e.next_due_date, e.building_id,
                   b.name  AS building_name,
                   c.company_id AS company_id,
                   c.name  AS company_name,
                   c.address AS company_address
            FROM Assignments a
            LEFT JOIN Users u_a        ON a.admin_id      = u_a.user_id
            LEFT JOIN Users u_i        ON a.inspector_id  = u_i.user_id
            LEFT JOIN Extinguishers e  ON a.extinguisher_id = e.extinguisher_id
            LEFT JOIN Buildings b      ON e.building_id   = b.building_id
            LEFT JOIN Companies c      ON b.company_id    = c.company_id
        """
        if user_id:
            rows = conn.execute(
                base + " WHERE a.inspector_id=? AND (a.org_id=? OR ? IS NULL)",
                (user_id, org_id, org_id)
            ).fetchall()
        else:
            rows = conn.execute(
                base + " WHERE (a.org_id=? OR ? IS NULL)",
                (org_id, org_id)
            ).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/assignments", methods=["POST"])
def create_assignment():
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Assignments (admin_id, inspector_id, extinguisher_id, due_date, notes, status, org_id) "
                "VALUES (?,?,?,?,?,'Pending Inspection',?)",
                (d.get("admin_id"), d.get("inspector_id"), d.get("extinguisher_id"),
                 d.get("due_date",""), d.get("notes",""), d.get("org_id"))
            )
        platform_log("admin", "assignment_created", "new", f"org_id={d.get('org_id')}")
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── UPDATE ASSIGNMENT ─────────────────────────────────────────────────────────
@app.route("/api/assignments/<int:assign_id>", methods=["PUT"])
def update_assignment(assign_id):
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "UPDATE Assignments SET inspector_id=?, extinguisher_id=?, "
                "due_date=?, notes=?, status=? WHERE assignment_id=?",
                (d.get("inspector_id"), d.get("extinguisher_id"),
                 d.get("due_date", ""), d.get("notes", ""),
                 d.get("status", "Pending Inspection"), assign_id)
            )
        platform_log("admin", "assignment_updated", str(assign_id), "Updated")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── REPORTS ────────────────────────────────────────────────────────────────────
@app.route("/api/reports", methods=["GET"])
def get_reports():
    """Fetch reports with full context.
    Client role: scoped to their company_id (only see their own building reports).
    Inspector/3rd Party Inspector: scoped to their own submitted reports.
    Admin: sees all org reports including Pending_Review from 3rd party.
    Approval status is always included so the UI can show quarantine state."""
    user_id          = request.args.get("user_id")
    org_id           = request.args.get("org_id")
    client_company_id = request.args.get("client_company_id")  # Client role scoping
    approval_filter  = request.args.get("approval_status")      # optional: Approved/Pending_Review/Void

    with get_db() as conn:
        base = """
            SELECT r.report_id, r.extinguisher_id, r.inspection_date,
                   r.notes, r.photo_path,
                   r.approval_status, r.reviewed_by, r.reviewed_at,
                   r.void_reason, r.po_number,
                   r.service_type, r.inspection_result,
                   r.chk_mounted, r.chk_access, r.chk_pressure, r.chk_seal,
                   r.chk_damage, r.chk_nozzle, r.chk_label, r.chk_weight,
                   u.username      AS inspector,
                   u.first_name    AS inspector_first_name,
                   u.last_name     AS inspector_last_name,
                   u.cert_number   AS inspector_cert,
                   u.subcontractor_company_name AS inspector_subcontractor,
                   u.role          AS inspector_role,
                   u_r.username    AS reviewed_by_name,
                   e.floor_number, e.room_number, e.location_description,
                   e.next_due_date, e.building_id,
                   b.name          AS building_name,
                   c.name          AS company_name,
                   c.company_id    AS company_id,
                   c.address       AS company_address
            FROM Reports r
            LEFT JOIN Users u         ON r.inspector_id    = u.user_id
            LEFT JOIN Users u_r       ON r.reviewed_by     = u_r.user_id
            LEFT JOIN Extinguishers e ON r.extinguisher_id = e.extinguisher_id
            LEFT JOIN Buildings b     ON e.building_id     = b.building_id
            LEFT JOIN Companies c     ON b.company_id      = c.company_id
        """
        conditions = []
        params = []

        if client_company_id:
            # Client: only see APPROVED reports for their company
            conditions.append("c.company_id = ?")
            params.append(client_company_id)
            conditions.append("(r.approval_status = 'Approved' OR r.approval_status IS NULL)")
        elif user_id:
            conditions.append("r.inspector_id = ?")
            params.append(user_id)

        if org_id and not client_company_id:
            conditions.append("(r.org_id = ? OR ? IS NULL)")
            params.extend([org_id, org_id])

        if approval_filter:
            conditions.append("r.approval_status = ?")
            params.append(approval_filter)

        where = (" WHERE " + " AND ".join(conditions)) if conditions else ""
        rows = conn.execute(base + where + " ORDER BY r.inspection_date DESC", params).fetchall()

    return jsonify([dict(r) for r in rows])

@app.route("/api/reports", methods=["POST"])
def create_report():
    """Submit an inspection report.
    Creates the Report row, then updates the parent Extinguisher with:
    - last_report_id: links to this report for the detail popup
    - last_inspection_date: used by Priority Queue for urgency sorting
    If an assignment_id is provided, marks that assignment as Complete."""
    d = request.get_json()
    try:
        with get_db() as conn:
            report_cols = [r[1] for r in conn.execute("PRAGMA table_info(Reports)").fetchall()]
            if "photo_path" not in report_cols:
                conn.execute("ALTER TABLE Reports ADD COLUMN photo_path TEXT")
            # Determine approval status based on submitting inspector's role
            # 3rd Party Inspector reports go into quarantine (Pending_Review)
            # until a Primary Admin counter-signs them
            inspector_id   = d.get("inspector_id")
            approval_status = "Approved"
            if inspector_id:
                insp = conn.execute(
                    "SELECT role FROM Users WHERE user_id=?", (inspector_id,)
                ).fetchone()
                if insp and insp["role"] == "3rd_Party_Inspector":
                    approval_status = "Pending_Review"

            service_type = d.get("service_type", "Routine Inspection")
            insp_date    = d.get("inspection_date")

            conn.execute(
                "INSERT INTO Reports (extinguisher_id, inspector_id, inspection_date, notes, photo_path, org_id, service_type,"
                " chk_mounted, chk_access, chk_pressure, chk_seal, chk_damage, chk_nozzle, chk_label, chk_weight,"
                " inspection_result, approval_status, po_number) "
                "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (d.get("extinguisher_id"), inspector_id,
                 insp_date, d.get("notes",""),
                 d.get("photo_path"), d.get("org_id"), service_type,
                 1 if d.get("chk_mounted") else 0,
                 1 if d.get("chk_access") else 0,
                 1 if d.get("chk_pressure") else 0,
                 1 if d.get("chk_seal") else 0,
                 1 if d.get("chk_damage") else 0,
                 1 if d.get("chk_nozzle") else 0,
                 1 if d.get("chk_label") else 0,
                 1 if d.get("chk_weight") else 0,
                 d.get("inspection_result", ""),
                 approval_status,
                 d.get("po_number", ""))
            )

            # Update extinguisher service history dates based on service type
            # Annual Maintenance → last_annual_date
            # 6-Year Service → last_6year_date
            # Hydrostatic Test → last_hydro_date
            ext_id = d.get("extinguisher_id")
            if insp_date and ext_id:
                if service_type == "Annual Maintenance":
                    conn.execute("UPDATE Extinguishers SET last_annual_date=? WHERE extinguisher_id=?",
                                 (insp_date, ext_id))
                elif service_type == "6-Year Service":
                    conn.execute("UPDATE Extinguishers SET last_6year_date=? WHERE extinguisher_id=?",
                                 (insp_date, ext_id))
                elif service_type == "Hydrostatic Test":
                    conn.execute("UPDATE Extinguishers SET last_hydro_date=? WHERE extinguisher_id=?",
                                 (insp_date, ext_id))
            # ── NFPA 10 Automated Interval Calculation ────────────────────────
            # When a report is submitted, automatically set next_due_date based
            # on the NFPA 10 service interval for the type of work performed.
            # This overrides any manually-set interval so the app stays compliant.
            NFPA_INTERVALS = {
                "Routine Inspection":  30,       # Monthly visual check
                "Annual Maintenance":  365,      # Annual internal maintenance
                "6-Year Service":      365 * 6,  # 6-year internal exam (2190 days)
                "Hydrostatic Test":    365 * 12, # 12-year hydrostatic test (4380 days)
                "Recharge":            365,      # Post-discharge recharge = treat as annual
                "Condemn":             None,     # No future due date for condemned units
            }
            if insp_date and ext_id and service_type in NFPA_INTERVALS:
                interval_days = NFPA_INTERVALS[service_type]
                if interval_days is not None:
                    try:
                        base = datetime.strptime(insp_date, "%Y-%m-%d")
                        next_due = (base + timedelta(days=interval_days)).strftime("%Y-%m-%d")
                        conn.execute(
                            "UPDATE Extinguishers SET next_due_date=?, inspection_interval_days=? WHERE extinguisher_id=?",
                            (next_due, interval_days, ext_id)
                        )
                    except ValueError:
                        pass  # malformed date — skip silently

            # Update extinguisher's last_report_id and last inspection date
            report_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]
            conn.execute(
                "UPDATE Extinguishers SET last_report_id=?, last_inspection_date=? WHERE extinguisher_id=?",
                (report_id, d.get("inspection_date"), d.get("extinguisher_id"))
            )
            if d.get("assignment_id"):
                conn.execute(
                    "UPDATE Assignments SET status='Inspection Complete' WHERE assignment_id=?",
                    (d.get("assignment_id"),)
                )
        platform_log("inspector", "report_submitted", "new", f"ext_id={d.get('extinguisher_id')}")
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── USERS ──────────────────────────────────────────────────────────────────────
@app.route("/api/users", methods=["GET"])
def get_users():
    """List all users in an organization.
    Returns user_id, username, role, PII fields, account_status, and company_name for Clients.
    Passwords and password_hash are never returned."""
    org_id = request.args.get("org_id")
    with get_db() as conn:
        if org_id:
            rows = conn.execute(
                """SELECT u.user_id, u.username, u.role,
                          u.first_name, u.last_name, u.middle_name,
                          u.email, u.phone, u.cert_number, u.account_status,
                          u.company_id, c.name AS company_name,
                          u.org_id, o.name AS org_name
                   FROM Users u
                   LEFT JOIN Companies c ON c.company_id = u.company_id
                   LEFT JOIN Organizations o ON o.org_id = u.org_id
                   WHERE u.org_id=?""", (org_id,)
            ).fetchall()
        else:
            rows = conn.execute(
                """SELECT u.user_id, u.username, u.role,
                          u.first_name, u.last_name, u.middle_name,
                          u.email, u.phone, u.cert_number, u.account_status,
                          u.company_id, c.name AS company_name,
                          u.org_id, o.name AS org_name
                   FROM Users u
                   LEFT JOIN Companies c ON c.company_id = u.company_id
                   LEFT JOIN Organizations o ON o.org_id = u.org_id"""
            ).fetchall()
    return jsonify([dict(r) for r in rows])

# ── USER PREFERENCES ──────────────────────────────────────────────────────────
@app.route("/api/users/<int:user_id>/password", methods=["PUT"])
def change_password(user_id):
    d = request.get_json()
    current_pw = (d.get("current_password") or "").strip()
    new_pw     = (d.get("new_password") or "").strip()

    if not current_pw or not new_pw:
        return jsonify({"error": "Both current and new password are required."}), 400

    try:
        with get_db() as conn:
            # Verify current password matches
            row = conn.execute(
                "SELECT user_id FROM Users WHERE user_id = ? AND password_hash = ?",
                (user_id, current_pw)
            ).fetchone()
            if not row:
                return jsonify({"error": "Current password is incorrect."}), 401
            # Update to new password
            conn.execute(
                "UPDATE Users SET password_hash = ? WHERE user_id = ?",
                (new_pw, user_id)
            )
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/users/<int:user_id>/preferences", methods=["GET"])
def get_preferences(user_id):
    import json
    with get_db() as conn:
        existing = [r[1] for r in conn.execute("PRAGMA table_info(Users)").fetchall()]
        if "preferences" not in existing:
            conn.execute("ALTER TABLE Users ADD COLUMN preferences TEXT DEFAULT '{}'")
        row = conn.execute(
            "SELECT preferences FROM Users WHERE user_id = ?", (user_id,)
        ).fetchone()
    if row and row["preferences"]:
        try:
            return jsonify(json.loads(row["preferences"]))
        except:
            return jsonify({"theme": "dark"})
    return jsonify({"theme": "dark"})

@app.route("/api/users/<int:user_id>/preferences", methods=["PUT"])
def save_preferences(user_id):
    import json
    prefs = request.get_json()
    try:
        with get_db() as conn:
            existing = [r[1] for r in conn.execute("PRAGMA table_info(Users)").fetchall()]
            if "preferences" not in existing:
                conn.execute("ALTER TABLE Users ADD COLUMN preferences TEXT DEFAULT '{}'")
            conn.execute(
                "UPDATE Users SET preferences = ? WHERE user_id = ?",
                (json.dumps(prefs), user_id)
            )
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── CREATE USER ───────────────────────────────────────────────────────────────
@app.route("/api/users", methods=["POST"])
def create_user():
    d          = request.get_json()
    username   = (d.get("username")   or "").strip()
    password   = (d.get("password")   or "").strip()
    role       = (d.get("role")       or "").strip()
    org_id      = d.get("org_id")
    # PII fields — first_name, last_name, email mandatory; others optional
    first_name  = (d.get("first_name")  or "").strip()
    last_name   = (d.get("last_name")   or "").strip()
    middle_name = (d.get("middle_name") or "").strip()
    email       = (d.get("email")       or "").strip()
    phone       = (d.get("phone")       or "").strip()
    cert_number = (d.get("cert_number") or "").strip()
    # Enterprise fields
    company_id                = d.get("company_id")  # Client role scoping (nullable)
    subcontractor_company_name = (d.get("subcontractor_company_name") or "").strip()

    # Mandatory field validation
    if not username:
        return jsonify({"error": "Username is required."}), 400
    if not password:
        return jsonify({"error": "Password is required."}), 400
    if not role:
        return jsonify({"error": "Role is required."}), 400
    if not first_name:
        return jsonify({"error": "First name is required."}), 400
    if not last_name:
        return jsonify({"error": "Last name is required."}), 400
    if not email:
        return jsonify({"error": "Email address is required."}), 400
    if role == "Client" and not company_id:
        return jsonify({"error": "A company must be assigned to Client accounts."}), 400

    valid_roles = ["Admin", "Inspector", "3rd_Party_Admin", "3rd_Party_Inspector", "Client"]
    if role not in valid_roles:
        return jsonify({"error": f"Invalid role. Must be one of: {', '.join(valid_roles)}"}), 400

    try:
        with get_db() as conn:
            existing = conn.execute(
                "SELECT user_id FROM Users WHERE username=?", (username,)
            ).fetchone()
            if existing:
                return jsonify({"error": "Username already exists."}), 409
            conn.execute(
                """INSERT INTO Users
                   (username, password_hash, role, org_id,
                    first_name, last_name, middle_name, email, phone, cert_number,
                    company_id, subcontractor_company_name)
                   VALUES (?,?,?,?,?,?,?,?,?,?,?,?)""",
                (username, password, role, org_id,
                 first_name, last_name, middle_name, email, phone, cert_number,
                 company_id, subcontractor_company_name)
            )
        platform_log(username, "user_created", username, f"role={role}")
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── EDIT USER ─────────────────────────────────────────────────────────────────
@app.route("/api/users/<int:user_id>", methods=["PUT"])
def update_user(user_id):
    d        = request.get_json()
    role     = (d.get("role")     or "").strip()
    username = (d.get("username") or "").strip()
    new_pw   = (d.get("password") or "").strip()  # optional — only set if provided
    # PII fields — all optional on edit (admin may update one at a time)
    first_name  = d.get("first_name")
    last_name   = d.get("last_name")
    middle_name = d.get("middle_name")
    email       = d.get("email")
    phone       = d.get("phone")
    cert_number = d.get("cert_number")
    company_id                = d.get("company_id")           # Client scoping
    subcontractor_company_name = d.get("subcontractor_company_name")  # 3rd party PDF name

    valid_roles = ["Admin", "Inspector", "3rd_Party_Admin", "3rd_Party_Inspector", "Client"]
    if role and role not in valid_roles:
        return jsonify({"error": f"Invalid role. Must be one of: {', '.join(valid_roles)}"}), 400

    try:
        with get_db() as conn:
            existing = conn.execute(
                "SELECT user_id, username FROM Users WHERE user_id=?", (user_id,)
            ).fetchone()
            if not existing:
                return jsonify({"error": "User not found."}), 404

            if username and username != existing["username"]:
                dup = conn.execute(
                    "SELECT user_id FROM Users WHERE username=? AND user_id!=?",
                    (username, user_id)
                ).fetchone()
                if dup:
                    return jsonify({"error": "Username already exists."}), 409

            # Build dynamic UPDATE — only set fields that were provided
            updates = []
            params  = []
            if username:
                updates.append("username=?");      params.append(username)
            if role:
                updates.append("role=?");          params.append(role)
            if new_pw:
                updates.append("password_hash=?"); params.append(new_pw)
            if first_name  is not None:
                updates.append("first_name=?");    params.append(first_name.strip())
            if last_name   is not None:
                updates.append("last_name=?");     params.append(last_name.strip())
            if middle_name is not None:
                updates.append("middle_name=?");   params.append(middle_name.strip())
            if email       is not None:
                updates.append("email=?");         params.append(email.strip())
            if phone       is not None:
                updates.append("phone=?");         params.append(phone.strip())
            if cert_number is not None:
                updates.append("cert_number=?");   params.append(cert_number.strip())
            if company_id is not None:
                updates.append("company_id=?");    params.append(company_id or None)
            if subcontractor_company_name is not None:
                updates.append("subcontractor_company_name=?")
                params.append(subcontractor_company_name.strip())

            if updates:
                params.append(user_id)
                conn.execute(
                    f"UPDATE Users SET {', '.join(updates)} WHERE user_id=?", params
                )

        changed_fields = [u.split("=?")[0] for u in updates] if updates else []
        details = f"Changed: {', '.join(changed_fields)}" if changed_fields else "No changes"
        platform_log("admin", "user_updated", str(user_id), details)
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── DELETE USER ───────────────────────────────────────────────────────────────
@app.route("/api/users/<int:user_id>", methods=["DELETE"])
def delete_user(user_id):
    try:
        with get_db() as conn:
            # Don't allow deleting yourself
            # (The UI will enforce this too, but belt-and-suspenders)
            user = conn.execute("SELECT username, role FROM Users WHERE user_id=?", (user_id,)).fetchone()
            if not user:
                return jsonify({"error": "User not found."}), 404

            # Reassign their assignments to unassigned (null inspector)
            conn.execute("UPDATE Assignments SET inspector_id=NULL WHERE inspector_id=?", (user_id,))
            # Keep their reports for audit trail, but delete the user
            conn.execute("DELETE FROM Users WHERE user_id=?", (user_id,))
        platform_log("admin", "user_deleted", str(user_id), "Deleted")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── SEARCH ─────────────────────────────────────────────────────────────────────
@app.route("/api/search", methods=["GET"])
def search():
    """Global search across all org data — extinguishers, assignments, reports,
    users, companies, buildings. Uses LIKE queries with 2-char minimum.
    Results grouped by type with title, subtitle, badge, and raw data.
    Limits to 10 results per category to prevent slow responses."""
    """Search across all org data — extinguishers, assignments, reports, users, companies, buildings."""
    q = (request.args.get("q") or "").strip().lower()
    org_id = request.args.get("org_id")
    if not q or len(q) < 2:
        return jsonify([])

    results = []
    like = f"%{q}%"

    with get_db() as conn:
        # Extinguishers
        rows = conn.execute("""
            SELECT e.extinguisher_id, e.floor_number, e.room_number, e.location_description,
                   e.ext_type, e.serial_number, e.ext_status, e.next_due_date,
                   b.name AS building_name, c.name AS company_name
            FROM Extinguishers e
            LEFT JOIN Buildings b ON b.building_id = e.building_id
            LEFT JOIN Companies c ON c.company_id = b.company_id
            WHERE e.org_id=? AND (
                LOWER(e.serial_number) LIKE ? OR LOWER(e.location_description) LIKE ?
                OR LOWER(e.ext_type) LIKE ? OR LOWER(e.room_number) LIKE ?
                OR LOWER(b.name) LIKE ? OR LOWER(c.name) LIKE ?
                OR CAST(e.extinguisher_id AS TEXT) LIKE ?
                OR LOWER(e.ext_status) LIKE ?
            ) LIMIT 10
        """, (org_id, like, like, like, like, like, like, like, like)).fetchall()
        for r in rows:
            results.append({
                "type": "extinguisher", "id": r["extinguisher_id"],
                "title": f"Ext #{r['extinguisher_id']}" + (f" — {r['serial_number']}" if r["serial_number"] else ""),
                "subtitle": " · ".join(filter(None, [r["company_name"], r["building_name"],
                    f"Floor {r['floor_number']}" if r["floor_number"] else None,
                    f"Room {r['room_number']}" if r["room_number"] else None])),
                "badge": r["ext_status"] or "Active",
                "data": dict(r)
            })

        # Assignments
        rows = conn.execute("""
            SELECT a.assignment_id, a.status, a.due_date, a.notes,
                   u_admin.username AS assigned_by, u_insp.username AS inspector,
                   e.extinguisher_id, b.name AS building_name, c.name AS company_name
            FROM Assignments a
            LEFT JOIN Users u_admin ON a.admin_id = u_admin.user_id
            LEFT JOIN Users u_insp ON a.inspector_id = u_insp.user_id
            LEFT JOIN Extinguishers e ON a.extinguisher_id = e.extinguisher_id
            LEFT JOIN Buildings b ON e.building_id = b.building_id
            LEFT JOIN Companies c ON b.company_id = c.company_id
            WHERE a.org_id=? AND (
                LOWER(a.status) LIKE ? OR LOWER(a.notes) LIKE ?
                OR LOWER(u_insp.username) LIKE ? OR LOWER(u_admin.username) LIKE ?
                OR CAST(a.assignment_id AS TEXT) LIKE ?
            ) LIMIT 10
        """, (org_id, like, like, like, like, like)).fetchall()
        for r in rows:
            results.append({
                "type": "assignment", "id": r["assignment_id"],
                "title": f"Assignment #{r['assignment_id']}",
                "subtitle": " · ".join(filter(None, [f"Inspector: {r['inspector']}" if r["inspector"] else None,
                    r["company_name"], r["building_name"], f"Due: {r['due_date']}" if r["due_date"] else None])),
                "badge": r["status"] or "Pending",
                "data": dict(r)
            })

        # Reports
        rows = conn.execute("""
            SELECT r.report_id, r.inspection_date, r.notes,
                   u.username AS inspector, e.extinguisher_id,
                   b.name AS building_name, c.name AS company_name
            FROM Reports r
            LEFT JOIN Users u ON r.inspector_id = u.user_id
            LEFT JOIN Extinguishers e ON r.extinguisher_id = e.extinguisher_id
            LEFT JOIN Buildings b ON e.building_id = b.building_id
            LEFT JOIN Companies c ON b.company_id = c.company_id
            WHERE r.org_id=? AND (
                LOWER(r.notes) LIKE ? OR LOWER(u.username) LIKE ?
                OR CAST(r.report_id AS TEXT) LIKE ?
                OR r.inspection_date LIKE ?
            ) LIMIT 10
        """, (org_id, like, like, like, like)).fetchall()
        for r in rows:
            results.append({
                "type": "report", "id": r["report_id"],
                "title": f"Report #{r['report_id']}",
                "subtitle": " · ".join(filter(None, [f"Inspector: {r['inspector']}" if r["inspector"] else None,
                    r["inspection_date"], r["company_name"]])),
                "badge": "Report",
                "data": dict(r)
            })

        # Users
        rows = conn.execute("""
            SELECT user_id, username, role FROM Users
            WHERE org_id=? AND (LOWER(username) LIKE ? OR LOWER(role) LIKE ?)
            LIMIT 10
        """, (org_id, like, like)).fetchall()
        for r in rows:
            results.append({
                "type": "user", "id": r["user_id"],
                "title": r["username"],
                "subtitle": r["role"].replace("_", " "),
                "badge": r["role"].split("_")[0] if "_" in r["role"] else r["role"],
                "data": dict(r)
            })

        # Companies
        rows = conn.execute("""
            SELECT company_id, name, address, city, state FROM Companies
            WHERE org_id=? AND (LOWER(name) LIKE ? OR LOWER(address) LIKE ? OR LOWER(city) LIKE ?)
            LIMIT 5
        """, (org_id, like, like, like)).fetchall()
        for r in rows:
            results.append({
                "type": "company", "id": r["company_id"],
                "title": r["name"],
                "subtitle": " · ".join(filter(None, [r["address"], r["city"], r["state"]])),
                "badge": "Company",
                "data": dict(r)
            })

        # Buildings
        rows = conn.execute("""
            SELECT b.building_id, b.name, b.address, c.name AS company_name FROM Buildings b
            LEFT JOIN Companies c ON c.company_id = b.company_id
            WHERE b.org_id=? AND (LOWER(b.name) LIKE ? OR LOWER(b.address) LIKE ?)
            LIMIT 5
        """, (org_id, like, like)).fetchall()
        for r in rows:
            results.append({
                "type": "building", "id": r["building_id"],
                "title": r["name"],
                "subtitle": r["company_name"] or r["address"] or "",
                "badge": "Building",
                "data": dict(r)
            })

    return jsonify(results)

# ── NOTIFICATIONS ─────────────────────────────────────────────────────────────
@app.route("/api/notifications", methods=["GET"])
def get_notifications():
    from datetime import datetime, timedelta
    user_id = request.args.get("user_id")
    role    = request.args.get("role", "")
    org_id  = request.args.get("org_id")
    since   = (datetime.now() - timedelta(hours=24)).strftime("%Y-%m-%d %H:%M:%S")
    notifs  = []
    org_filter     = "AND (a.org_id=? OR ? IS NULL)"
    org_filter_r   = "AND (r.org_id=? OR ? IS NULL)"
    org_filter_e   = "AND (e.org_id=? OR ? IS NULL)"
    op = (org_id, org_id)  # reusable param pair

    with get_db() as conn:
        # ── New assignments ────────────────────────────────────────────────
        if role in ("Admin", "3rd_Party_Admin"):
            rows = conn.execute(
                "SELECT a.assignment_id, u_i.username AS inspector, a.extinguisher_id, a.due_date "
                "FROM Assignments a LEFT JOIN Users u_i ON a.inspector_id = u_i.user_id "
                f"WHERE 1=1 {org_filter} ORDER BY a.assignment_id DESC LIMIT 10", op
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT a.assignment_id, u_a.username AS admin, a.extinguisher_id, a.due_date "
                "FROM Assignments a LEFT JOIN Users u_a ON a.admin_id = u_a.user_id "
                f"WHERE a.inspector_id=? {org_filter} ORDER BY a.assignment_id DESC LIMIT 10",
                (user_id,) + op
            ).fetchall()
        for r in rows:
            msg = (f"Assignment #{r['assignment_id']} created for {r['inspector'] or 'Inspector'} — Ext #{r['extinguisher_id']}"
                   if role in ("Admin","3rd_Party_Admin")
                   else f"New assignment: Ext #{r['extinguisher_id']} assigned to you")
            notifs.append({"id": f"assign_new_{r['assignment_id']}", "type": "assignment_created",
                           "message": msg, "timestamp": r['due_date'] or since})

        # ── Completed assignments ──────────────────────────────────────────
        if role in ("Admin", "3rd_Party_Admin"):
            completed = conn.execute(
                "SELECT a.assignment_id, u_i.username AS inspector, a.extinguisher_id "
                "FROM Assignments a LEFT JOIN Users u_i ON a.inspector_id = u_i.user_id "
                f"WHERE a.status='Inspection Complete' {org_filter} ORDER BY a.assignment_id DESC LIMIT 5", op
            ).fetchall()
        else:
            completed = conn.execute(
                "SELECT a.assignment_id, a.extinguisher_id FROM Assignments a "
                f"WHERE a.status='Inspection Complete' AND a.inspector_id=? {org_filter} "
                "ORDER BY a.assignment_id DESC LIMIT 5", (user_id,) + op
            ).fetchall()
        for r in completed:
            notifs.append({"id": f"assign_complete_{r['assignment_id']}", "type": "assignment_complete",
                           "message": f"Assignment #{r['assignment_id']} marked complete — Ext #{r['extinguisher_id']}",
                           "timestamp": since})

        # ── Reports submitted ──────────────────────────────────────────────
        if role in ("Admin", "3rd_Party_Admin"):
            reports = conn.execute(
                "SELECT r.report_id, u.username AS inspector, r.extinguisher_id, r.inspection_date "
                "FROM Reports r LEFT JOIN Users u ON r.inspector_id = u.user_id "
                f"WHERE 1=1 {org_filter_r} ORDER BY r.report_id DESC LIMIT 10", op
            ).fetchall()
        else:
            reports = conn.execute(
                "SELECT r.report_id, r.extinguisher_id, r.inspection_date FROM Reports r "
                f"WHERE r.inspector_id=? {org_filter_r} ORDER BY r.report_id DESC LIMIT 10",
                (user_id,) + op
            ).fetchall()
        for r in reports:
            notifs.append({"id": f"report_{r['report_id']}", "type": "report_submitted",
                           "message": f"Inspection report #{r['report_id']} submitted for Ext #{r['extinguisher_id']}",
                           "timestamp": r['inspection_date'] or since})

        # ── Overdue extinguishers ──────────────────────────────────────────
        today = datetime.now().strftime("%Y-%m-%d")
        overdue = conn.execute(
            "SELECT extinguisher_id, address, next_due_date FROM Extinguishers e "
            f"WHERE next_due_date IS NOT NULL AND next_due_date<? {org_filter_e} "
            "ORDER BY next_due_date ASC LIMIT 10", (today,) + op
        ).fetchall()
        for r in overdue:
            notifs.append({"id": f"overdue_{r['extinguisher_id']}", "type": "overdue",
                           "message": f"Ext #{r['extinguisher_id']} at {r['address']} overdue since {r['next_due_date']}",
                           "timestamp": r['next_due_date']})

    notifs.sort(key=lambda x: x.get("timestamp",""), reverse=True)
    return jsonify(notifs)

# ── STATS ──────────────────────────────────────────────────────────────────────
@app.route("/api/stats", methods=["GET"])
def get_stats():
    org_id = request.args.get("org_id")
    with get_db() as conn:
        if org_id:
            ext     = conn.execute("SELECT COUNT(*) FROM Extinguishers WHERE org_id=?",  (org_id,)).fetchone()[0]
            assigns = conn.execute("SELECT COUNT(*) FROM Assignments   WHERE org_id=?",  (org_id,)).fetchone()[0]
            reports = conn.execute("SELECT COUNT(*) FROM Reports       WHERE org_id=?",  (org_id,)).fetchone()[0]
            users   = conn.execute("SELECT COUNT(*) FROM Users         WHERE org_id=?",  (org_id,)).fetchone()[0]
        else:
            ext     = conn.execute("SELECT COUNT(*) FROM Extinguishers").fetchone()[0]
            assigns = conn.execute("SELECT COUNT(*) FROM Assignments").fetchone()[0]
            reports = conn.execute("SELECT COUNT(*) FROM Reports").fetchone()[0]
            users   = conn.execute("SELECT COUNT(*) FROM Users").fetchone()[0]
    return jsonify({"extinguishers":ext, "assignments":assigns, "reports":reports, "users":users})

# ══════════════════════════════════════════════════════════════════════════════
#  PLATFORM SUPER ADMIN
# ══════════════════════════════════════════════════════════════════════════════

@app.route("/api/platform/login", methods=["POST"])
def platform_login():
    """Authenticate the Platform Super Admin with a separate password.
    This is independent from org-level user accounts.
    Logs both successful and failed attempts to PlatformAuditLog."""
    """Authenticate with the platform password (separate from org login)."""
    d = request.get_json()
    password = (d.get("password") or "").strip()
    if not password:
        return jsonify({"error": "Password is required."}), 400

    # Compare SHA-256 hash (client sends pre-hashed, same as org login)
    if password == PLATFORM_PASSWORD_HASH:
        platform_log("SuperAdmin", "platform_login", None, "Platform admin logged in")
        return jsonify({"success": True, "role": "SuperAdmin"})
    platform_log("unknown", "platform_login_failed", None, "Failed platform login attempt")
    return jsonify({"error": "Invalid platform password."}), 401

@app.route("/api/platform/organizations", methods=["GET"])
def platform_list_orgs():
    """List all organizations with stats for the Super Admin dashboard."""
    with get_db() as conn:
        orgs = conn.execute("""
            SELECT o.org_id, o.name, o.address, o.city, o.state, o.phone,
                   o.contact_name, o.created_at, o.platform_status, o.last_active,
                   COUNT(DISTINCT u.user_id)           AS user_count,
                   COUNT(DISTINCT e.extinguisher_id)   AS extinguisher_count,
                   COUNT(DISTINCT r.report_id)         AS report_count,
                   (SELECT COUNT(*) FROM Extinguishers e2
                    WHERE e2.org_id = o.org_id
                      AND e2.ext_status NOT IN ('Condemned/Removed','Retired')
                      AND (e2.next_due_date IS NULL OR e2.next_due_date >= date('now'))
                   ) AS compliant_count,
                   (SELECT COUNT(*) FROM Extinguishers e2
                    WHERE e2.org_id = o.org_id
                      AND e2.ext_status NOT IN ('Condemned/Removed','Retired','Out of Service','Missing')
                      AND e2.next_due_date IS NOT NULL
                      AND e2.next_due_date < date('now')
                   ) AS overdue_count,
                   (SELECT COUNT(*) FROM Extinguishers e2
                    WHERE e2.org_id = o.org_id
                      AND e2.ext_status IN ('Out of Service','Missing')
                   ) AS non_compliant_count,
                   (SELECT COUNT(*) FROM Extinguishers e2
                    WHERE e2.org_id = o.org_id
                      AND e2.ext_status NOT IN ('Condemned/Removed','Retired')
                   ) AS active_ext_total
            FROM Organizations o
            LEFT JOIN Users u ON u.org_id = o.org_id
            LEFT JOIN Extinguishers e ON e.org_id = o.org_id
            LEFT JOIN Reports r ON r.org_id = o.org_id
            GROUP BY o.org_id
            ORDER BY o.created_at DESC
        """).fetchall()
    return jsonify([dict(r) for r in orgs])

@app.route("/api/platform/organizations/<int:org_id>/status", methods=["PUT"])
def platform_update_org_status(org_id):
    """Approve, reject, or suspend an organization."""
    d = request.get_json()
    new_status = (d.get("status") or "").strip()
    valid = ["approved", "pending", "suspended", "rejected"]
    if new_status not in valid:
        return jsonify({"error": f"Status must be one of: {', '.join(valid)}"}), 400
    try:
        with get_db() as conn:
            org = conn.execute("SELECT org_id FROM Organizations WHERE org_id=?", (org_id,)).fetchone()
            if not org:
                return jsonify({"error": "Organization not found."}), 404
            conn.execute("UPDATE Organizations SET platform_status=? WHERE org_id=?", (new_status, org_id))
            org_name = conn.execute("SELECT name FROM Organizations WHERE org_id=?", (org_id,)).fetchone()
            platform_log("SuperAdmin", f"org_{new_status}", org_name["name"] if org_name else str(org_id), f"Changed org #{org_id} to {new_status}")
        return jsonify({"success": True, "status": new_status})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/platform/organizations/<int:org_id>", methods=["DELETE"])
def platform_delete_org(org_id):
    """Permanently delete an organization and all its data."""
    try:
        with get_db() as conn:
            org = conn.execute("SELECT name FROM Organizations WHERE org_id=?", (org_id,)).fetchone()
            if not org:
                return jsonify({"error": "Organization not found."}), 404
            # Cascade delete all org data
            conn.execute("DELETE FROM Reports      WHERE org_id=?", (org_id,))
            conn.execute("DELETE FROM Assignments   WHERE org_id=?", (org_id,))
            conn.execute("DELETE FROM Extinguishers WHERE org_id=?", (org_id,))
            conn.execute("DELETE FROM Buildings     WHERE org_id=?", (org_id,))
            conn.execute("DELETE FROM Companies     WHERE org_id=?", (org_id,))
            conn.execute("DELETE FROM Users         WHERE org_id=?", (org_id,))
            conn.execute("DELETE FROM Organizations WHERE org_id=?", (org_id,))
        platform_log("SuperAdmin", "org_deleted", org["name"], f"Permanently deleted org #{org_id} and all data")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/platform/settings", methods=["GET"])
def platform_get_settings():
    """Get platform settings."""
    with get_db() as conn:
        rows = conn.execute("SELECT key, value FROM PlatformSettings").fetchall()
    settings = {}
    for r in rows:
        val = r["value"]
        if val == "true": val = True
        elif val == "false": val = False
        settings[r["key"]] = val
    return jsonify(settings)

@app.route("/api/platform/settings", methods=["PUT"])
def platform_update_settings():
    """Update platform settings."""
    d = request.get_json()
    try:
        with get_db() as conn:
            for key, value in d.items():
                str_val = "true" if value is True else "false" if value is False else str(value)
                existing = conn.execute("SELECT key FROM PlatformSettings WHERE key=?", (key,)).fetchone()
                if existing:
                    conn.execute("UPDATE PlatformSettings SET value=? WHERE key=?", (str_val, key))
                else:
                    conn.execute("INSERT INTO PlatformSettings (key, value) VALUES (?,?)", (key, str_val))
        for key, value in d.items():
            platform_log("SuperAdmin", "settings_changed", key, f"Set {key} = {value}")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── PLATFORM BACKGROUND ────────────────────────────────────────────────────────
# Background image settings are stored in PlatformSettings under three keys:
#   platform_bg_url      — filename of the uploaded image (served from /backgrounds/)
#   platform_bg_opacity  — integer 5–40 (percent)
#   platform_bg_enabled  — "true" / "false"

def _get_bg_setting(conn, key, default=""):
    row = conn.execute("SELECT value FROM PlatformSettings WHERE key=?", (key,)).fetchone()
    return row["value"] if row else default

def _set_bg_setting(conn, key, value):
    exists = conn.execute("SELECT key FROM PlatformSettings WHERE key=?", (key,)).fetchone()
    if exists:
        conn.execute("UPDATE PlatformSettings SET value=? WHERE key=?", (str(value), key))
    else:
        conn.execute("INSERT INTO PlatformSettings (key, value) VALUES (?,?)", (key, str(value)))

@app.route("/api/platform/background", methods=["GET"])
def platform_get_background():
    """Return current platform background settings (public — called by all clients on load)."""
    with get_db() as conn:
        url     = _get_bg_setting(conn, "platform_bg_url",     "")
        opacity = _get_bg_setting(conn, "platform_bg_opacity", "10")
        enabled = _get_bg_setting(conn, "platform_bg_enabled", "false")
    return jsonify({
        "url":     f"/backgrounds/{url}" if url else "",
        "opacity": int(opacity),
        "enabled": enabled == "true"
    })

@app.route("/api/platform/background", methods=["POST"])
def platform_upload_background():
    """Upload a new platform background image.
    Accepts multipart/form-data with fields:
      - image  : the image file
      - opacity: integer 5–40 (optional, defaults to 10)
      - enabled: 'true'/'false' (optional, defaults to 'true')
    Deletes any previously stored background file to avoid orphans."""
    if "image" not in request.files:
        return jsonify({"error": "No image file provided"}), 400
    file = request.files["image"]
    if not file.filename:
        return jsonify({"error": "Empty filename"}), 400
    ext = file.filename.rsplit(".", 1)[-1].lower() if "." in file.filename else ""
    if ext not in BACKGROUND_EXTENSIONS:
        return jsonify({"error": f"File type .{ext} not allowed. Use: jpg, png, webp, svg, gif"}), 400

    opacity = request.form.get("opacity", "10")
    enabled = request.form.get("enabled", "true")

    try:
        opacity = max(5, min(40, int(opacity)))
    except (ValueError, TypeError):
        opacity = 10

    # Delete old background file if one exists
    with get_db() as conn:
        old_file = _get_bg_setting(conn, "platform_bg_url", "")
    if old_file:
        old_path = os.path.join(BACKGROUND_DIR, old_file)
        if os.path.exists(old_path):
            try:
                os.remove(old_path)
            except Exception:
                pass

    # Save new file with UUID name
    unique_name = f"bg_{uuid.uuid4().hex}.{ext}"
    file.save(os.path.join(BACKGROUND_DIR, unique_name))

    with get_db() as conn:
        _set_bg_setting(conn, "platform_bg_url",     unique_name)
        _set_bg_setting(conn, "platform_bg_opacity", str(opacity))
        _set_bg_setting(conn, "platform_bg_enabled", enabled)

    platform_log("SuperAdmin", "background_updated", "platform_bg", f"Uploaded {unique_name}, opacity={opacity}")
    return jsonify({
        "success": True,
        "url":     f"/backgrounds/{unique_name}",
        "opacity": opacity,
        "enabled": enabled == "true"
    }), 201

@app.route("/api/platform/background", methods=["PUT"])
def platform_update_background_settings():
    """Update opacity and/or enabled flag without re-uploading the image."""
    d = request.get_json() or {}
    with get_db() as conn:
        if "opacity" in d:
            opacity = max(5, min(40, int(d["opacity"])))
            _set_bg_setting(conn, "platform_bg_opacity", str(opacity))
        if "enabled" in d:
            _set_bg_setting(conn, "platform_bg_enabled", "true" if d["enabled"] else "false")
        url     = _get_bg_setting(conn, "platform_bg_url",     "")
        opacity = int(_get_bg_setting(conn, "platform_bg_opacity", "10"))
        enabled = _get_bg_setting(conn, "platform_bg_enabled", "false") == "true"
    platform_log("SuperAdmin", "background_settings_changed", "platform_bg", str(d))
    return jsonify({"success": True, "url": f"/backgrounds/{url}" if url else "", "opacity": opacity, "enabled": enabled})

@app.route("/api/platform/background", methods=["DELETE"])
def platform_delete_background():
    """Remove the platform background image and clear all settings."""
    with get_db() as conn:
        old_file = _get_bg_setting(conn, "platform_bg_url", "")
        _set_bg_setting(conn, "platform_bg_url",     "")
        _set_bg_setting(conn, "platform_bg_enabled", "false")
    if old_file:
        old_path = os.path.join(BACKGROUND_DIR, old_file)
        if os.path.exists(old_path):
            try:
                os.remove(old_path)
            except Exception:
                pass
    platform_log("SuperAdmin", "background_removed", "platform_bg", "Background cleared")
    return jsonify({"success": True})


def _req_int(source, key):
    """Helper: pull an integer from a dict or return None."""
    try:
        v = source.get(key)
        return int(v) if v is not None else None
    except (TypeError, ValueError):
        return None


# ── EMAIL HELPER ──────────────────────────────────────────────────────────────
def send_email(to_address, subject, html_body, text_body=None):
    """Send an email via Gmail SMTP. Returns (True, None) on success or (False, error_str) on failure."""
    if not GMAIL_USER or not GMAIL_APP_PASS:
        return False, "Email not configured (set GMAIL_USER and GMAIL_APP_PASSWORD env vars)"
    try:
        msg = MIMEMultipart("alternative")
        msg["Subject"] = subject
        msg["From"]    = f"FireWatch <{GMAIL_USER}>"
        msg["To"]      = to_address
        if text_body:
            msg.attach(MIMEText(text_body, "plain"))
        msg.attach(MIMEText(html_body, "html"))
        ctx = ssl.create_default_context()
        with smtplib.SMTP_SSL("smtp.gmail.com", 465, context=ctx) as server:
            server.login(GMAIL_USER, GMAIL_APP_PASS)
            server.sendmail(GMAIL_USER, to_address, msg.as_string())
        return True, None
    except Exception as e:
        return False, str(e)


# ── PASSWORD RESET ─────────────────────────────────────────────────────────────
@app.route("/api/auth/forgot-password", methods=["POST"])
def forgot_password():
    data  = request.get_json(force=True) or {}
    email = (data.get("email") or "").strip().lower()
    if not email:
        return jsonify({"error": "Email required"}), 400
    with get_db() as conn:
        user = conn.execute(
            "SELECT user_id, full_name FROM Users WHERE LOWER(email)=? AND is_active=1",
            (email,)
        ).fetchone()
        if not user:
            # Don't reveal whether the email exists
            return jsonify({"message": "If that email exists, a reset link has been sent."}), 200
        # Invalidate any existing tokens
        conn.execute(
            "UPDATE PasswordResetTokens SET used=1 WHERE user_id=? AND used=0",
            (user["user_id"],)
        )
        token      = secrets.token_urlsafe(32)
        expires_at = (datetime.utcnow() + timedelta(hours=2)).strftime("%Y-%m-%d %H:%M:%S")
        conn.execute(
            "INSERT INTO PasswordResetTokens (user_id, token, expires_at) VALUES (?,?,?)",
            (user["user_id"], token, expires_at)
        )
    reset_url = f"{APP_BASE_URL}/reset-password?token={token}"
    html_body = f"""
    <div style="font-family:Arial,sans-serif;max-width:560px;margin:0 auto;">
      <h2 style="color:#e27c00;">🔥 FireWatch Password Reset</h2>
      <p>Hi {user['full_name']},</p>
      <p>We received a request to reset your FireWatch password. Click the button below to set a new password. This link expires in <strong>2 hours</strong>.</p>
      <p style="text-align:center;margin:32px 0;">
        <a href="{reset_url}"
           style="background:#e27c00;color:#fff;padding:14px 28px;border-radius:6px;text-decoration:none;font-weight:bold;font-size:16px;">
          Reset My Password
        </a>
      </p>
      <p style="color:#888;font-size:13px;">If you didn't request this, you can safely ignore this email. Your password won't change.</p>
      <hr style="border:none;border-top:1px solid #eee;margin:24px 0;">
      <p style="color:#aaa;font-size:12px;">FireWatch — NFPA 10 Compliance Platform</p>
    </div>
    """
    ok, err = send_email(email, "Reset your FireWatch password", html_body)
    if not ok:
        app.logger.error(f"Password reset email failed: {err}")
    return jsonify({"message": "If that email exists, a reset link has been sent."}), 200


@app.route("/api/auth/reset-password", methods=["POST"])
def reset_password():
    data     = request.get_json(force=True) or {}
    token    = (data.get("token") or "").strip()
    password = (data.get("password") or "").strip()
    if not token or not password:
        return jsonify({"error": "Token and new password required"}), 400
    if len(password) < 6:
        return jsonify({"error": "Password must be at least 6 characters"}), 400
    with get_db() as conn:
        row = conn.execute(
            """SELECT t.token_id, t.user_id, t.expires_at, t.used
               FROM PasswordResetTokens t
               WHERE t.token=?""",
            (token,)
        ).fetchone()
        if not row:
            return jsonify({"error": "Invalid or expired reset link"}), 400
        if row["used"]:
            return jsonify({"error": "This reset link has already been used"}), 400
        if datetime.utcnow() > datetime.strptime(row["expires_at"], "%Y-%m-%d %H:%M:%S"):
            return jsonify({"error": "Reset link has expired. Please request a new one."}), 400
        hashed = hash_password(password)
        conn.execute("UPDATE Users SET password_hash=? WHERE user_id=?", (hashed, row["user_id"]))
        conn.execute("UPDATE PasswordResetTokens SET used=1 WHERE token_id=?", (row["token_id"],))
    return jsonify({"message": "Password updated successfully"}), 200


# ── GENERAL EMAIL SEND (admin/marshal compose) ────────────────────────────────
@app.route("/api/email/send", methods=["POST"])
def email_send():
    data    = request.get_json(force=True) or {}
    to_addr = (data.get("to") or "").strip()
    subject = (data.get("subject") or "").strip()
    body    = (data.get("body") or "").strip()
    role    = (data.get("role") or "").strip()
    if not to_addr or not subject or not body:
        return jsonify({"error": "to, subject, and body are required"}), 400
    # Only admins, marshals, and platform admins may send
    allowed_roles = {"Admin", "FireMarshal", "PlatformAdmin"}
    if role not in allowed_roles:
        return jsonify({"error": "Insufficient permissions"}), 403
    html_body = f"""
    <div style="font-family:Arial,sans-serif;max-width:560px;margin:0 auto;">
      <h2 style="color:#e27c00;">🔥 FireWatch Message</h2>
      <div style="white-space:pre-wrap;">{body}</div>
      <hr style="border:none;border-top:1px solid #eee;margin:24px 0;">
      <p style="color:#aaa;font-size:12px;">FireWatch — NFPA 10 Compliance Platform</p>
    </div>
    """
    ok, err = send_email(to_addr, subject, html_body, text_body=body)
    if not ok:
        return jsonify({"error": f"Failed to send email: {err}"}), 500
    return jsonify({"message": "Email sent"}), 200


# ── MESSAGING ─────────────────────────────────────────────────────────────────
@app.route("/api/messages", methods=["GET"])
def get_messages():
    """Inbox: messages sent to this user (or broadcasts to their role).
    Requires query params: user_id, org_id, role"""
    uid  = _req_int(request.args, "user_id")
    oid  = _req_int(request.args, "org_id")
    role = (request.args.get("role") or "").strip()
    if not uid or not oid:
        return jsonify({"error": "user_id and org_id required"}), 400
    with get_db() as conn:
        rows = conn.execute(
            """SELECT m.message_id, m.subject, m.body, m.is_read, m.is_broadcast,
                      m.broadcast_role, m.created_at,
                      (u.first_name || ' ' || u.last_name) AS sender_name, u.email AS sender_email
               FROM Messages m
               JOIN Users u ON u.user_id = m.sender_id
               WHERE m.org_id = ?
                 AND (
                   (m.is_broadcast = 0 AND m.recipient_id = ?)
                   OR
                   (m.is_broadcast = 1 AND (m.broadcast_role = '' OR m.broadcast_role = ?))
                 )
               ORDER BY m.created_at DESC
               LIMIT 200""",
            (oid, uid, role)
        ).fetchall()
    return jsonify([dict(r) for r in rows]), 200


@app.route("/api/messages/sent", methods=["GET"])
def get_sent_messages():
    """Messages sent by this user. Requires query params: user_id, org_id"""
    uid = _req_int(request.args, "user_id")
    oid = _req_int(request.args, "org_id")
    if not uid or not oid:
        return jsonify({"error": "user_id and org_id required"}), 400
    with get_db() as conn:
        rows = conn.execute(
            """SELECT m.message_id, m.subject, m.body, m.is_read, m.is_broadcast,
                      m.broadcast_role, m.created_at,
                      (u.first_name || ' ' || u.last_name) AS recipient_name, u.email AS recipient_email
               FROM Messages m
               LEFT JOIN Users u ON u.user_id = m.recipient_id
               WHERE m.org_id = ? AND m.sender_id = ?
               ORDER BY m.created_at DESC
               LIMIT 200""",
            (oid, uid)
        ).fetchall()
    return jsonify([dict(r) for r in rows]), 200


@app.route("/api/messages/unread", methods=["GET"])
def get_unread_count():
    """Requires query params: user_id, org_id, role"""
    uid  = _req_int(request.args, "user_id")
    oid  = _req_int(request.args, "org_id")
    role = (request.args.get("role") or "").strip()
    if not uid or not oid:
        return jsonify({"unread": 0}), 200
    with get_db() as conn:
        row = conn.execute(
            """SELECT COUNT(*) AS cnt FROM Messages m
               WHERE m.org_id = ?
                 AND m.is_read = 0
                 AND (
                   (m.is_broadcast = 0 AND m.recipient_id = ?)
                   OR
                   (m.is_broadcast = 1 AND (m.broadcast_role = '' OR m.broadcast_role = ?))
                 )""",
            (oid, uid, role)
        ).fetchone()
    return jsonify({"unread": row["cnt"]}), 200


@app.route("/api/messages", methods=["POST"])
def send_message():
    """Send a DM or broadcast. Body must include: user_id, org_id, role, subject, body.
    For DM also include: recipient_id. For broadcast: is_broadcast=true, broadcast_role (optional)."""
    data    = request.get_json(force=True) or {}
    subject = (data.get("subject") or "").strip()
    body    = (data.get("body") or "").strip()
    if not subject or not body:
        return jsonify({"error": "subject and body required"}), 400
    uid  = _req_int(data, "user_id")
    oid  = _req_int(data, "org_id")
    role = (data.get("role") or "").strip()
    if not uid or not oid:
        return jsonify({"error": "user_id and org_id required"}), 400
    is_broadcast   = int(bool(data.get("is_broadcast")))
    broadcast_role = (data.get("broadcast_role") or "").strip() if is_broadcast else ""
    recipient_id   = _req_int(data, "recipient_id") if not is_broadcast else None
    # Admins/marshals can broadcast; everyone can DM
    if is_broadcast and role not in {"Admin", "FireMarshal"}:
        return jsonify({"error": "Only Admins and FireMarshals can broadcast"}), 403
    with get_db() as conn:
        if not is_broadcast and recipient_id:
            r = conn.execute(
                "SELECT user_id FROM Users WHERE user_id=? AND org_id=?",
                (recipient_id, oid)
            ).fetchone()
            if not r:
                return jsonify({"error": "Recipient not found in your organization"}), 404
        conn.execute(
            """INSERT INTO Messages
               (org_id, sender_id, recipient_id, subject, body, is_broadcast, broadcast_role)
               VALUES (?,?,?,?,?,?,?)""",
            (oid, uid, recipient_id, subject, body, is_broadcast, broadcast_role)
        )
        msg_id = conn.execute("SELECT last_insert_rowid() AS id").fetchone()["id"]
    return jsonify({"message_id": msg_id, "message": "Sent"}), 201


@app.route("/api/messages/<int:msg_id>/read", methods=["PUT"])
def mark_message_read(msg_id):
    """Mark a message as read. Body must include: user_id, org_id, role"""
    data = request.get_json(force=True) or {}
    uid  = _req_int(data, "user_id")
    oid  = _req_int(data, "org_id")
    role = (data.get("role") or "").strip()
    if not uid or not oid:
        return jsonify({"error": "user_id and org_id required"}), 400
    with get_db() as conn:
        row = conn.execute(
            """SELECT * FROM Messages WHERE message_id=? AND org_id=?
               AND (
                 (is_broadcast=0 AND recipient_id=?)
                 OR (is_broadcast=1 AND (broadcast_role='' OR broadcast_role=?))
               )""",
            (msg_id, oid, uid, role)
        ).fetchone()
        if not row:
            return jsonify({"error": "Message not found"}), 404
        conn.execute("UPDATE Messages SET is_read=1 WHERE message_id=?", (msg_id,))
    return jsonify({"message": "Marked read"}), 200


@app.route("/api/messages/<int:msg_id>", methods=["DELETE"])
def delete_message(msg_id):
    """Delete a message. Body must include: user_id, org_id"""
    data = request.get_json(force=True) or {}
    uid  = _req_int(data, "user_id")
    oid  = _req_int(data, "org_id")
    if not uid or not oid:
        return jsonify({"error": "user_id and org_id required"}), 400
    with get_db() as conn:
        row = conn.execute(
            "SELECT * FROM Messages WHERE message_id=? AND org_id=? AND (recipient_id=? OR sender_id=?)",
            (msg_id, oid, uid, uid)
        ).fetchone()
        if not row:
            return jsonify({"error": "Message not found"}), 404
        conn.execute("DELETE FROM Messages WHERE message_id=?", (msg_id,))
    return jsonify({"message": "Deleted"}), 200


@app.route("/api/messages/users", methods=["GET"])
def get_messageable_users():
    """List org users this user can message (excluding themselves).
    Requires query params: user_id, org_id"""
    uid = _req_int(request.args, "user_id")
    oid = _req_int(request.args, "org_id")
    if not uid or not oid:
        return jsonify({"error": "user_id and org_id required"}), 400
    with get_db() as conn:
        rows = conn.execute(
            """SELECT user_id,
                      (first_name || ' ' || last_name) AS full_name,
                      email, role
               FROM Users WHERE org_id=? AND user_id!=?
               ORDER BY first_name, last_name""",
            (oid, uid)
        ).fetchall()
    return jsonify([dict(r) for r in rows]), 200


# ── Audit log helper ──────────────────────────────────────────────────────────
def platform_log(actor, action, target=None, details=None):
    """Record a platform-level action to the audit log."""
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO PlatformAuditLog (actor, action, target, details) VALUES (?,?,?,?)",
                (actor, action, target, details)
            )
    except:
        pass  # don't break the main action if logging fails

@app.route("/api/platform/organizations/<int:org_id>/details", methods=["GET"])
def platform_org_details(org_id):
    """Drill into an org — returns users, extinguishers, assignments, reports."""
    with get_db() as conn:
        org = conn.execute("SELECT * FROM Organizations WHERE org_id=?", (org_id,)).fetchone()
        if not org:
            return jsonify({"error": "Organization not found."}), 404

        users = conn.execute(
            "SELECT user_id, username, role FROM Users WHERE org_id=?", (org_id,)
        ).fetchall()

        extinguishers = conn.execute("""
            SELECT e.extinguisher_id, e.floor_number, e.room_number,
                   e.location_description, e.ext_type, e.ext_status,
                   e.serial_number, e.next_due_date,
                   b.name AS building_name, c.name AS company_name
            FROM Extinguishers e
            LEFT JOIN Buildings b ON b.building_id = e.building_id
            LEFT JOIN Companies c ON c.company_id  = b.company_id
            WHERE e.org_id=?
            ORDER BY c.name, b.name, e.floor_number
        """, (org_id,)).fetchall()

        assignments = conn.execute("""
            SELECT a.assignment_id, a.status, a.due_date, a.notes,
                   u_admin.username AS assigned_by,
                   u_insp.username AS inspector
            FROM Assignments a
            LEFT JOIN Users u_admin ON a.admin_id = u_admin.user_id
            LEFT JOIN Users u_insp  ON a.inspector_id = u_insp.user_id
            WHERE a.org_id=?
            ORDER BY a.due_date DESC
        """, (org_id,)).fetchall()

        reports = conn.execute("""
            SELECT r.report_id, r.inspection_date, r.notes,
                   u.username AS inspector,
                   e.extinguisher_id, e.location_description
            FROM Reports r
            LEFT JOIN Users u ON r.inspector_id = u.user_id
            LEFT JOIN Extinguishers e ON r.extinguisher_id = e.extinguisher_id
            WHERE r.org_id=?
            ORDER BY r.inspection_date DESC
        """, (org_id,)).fetchall()

        companies = conn.execute(
            "SELECT company_id, name, address, city, state FROM Companies WHERE org_id=?", (org_id,)
        ).fetchall()

        buildings = conn.execute("""
            SELECT b.building_id, b.name, b.address, b.floors,
                   c.name AS company_name
            FROM Buildings b
            LEFT JOIN Companies c ON c.company_id = b.company_id
            WHERE b.org_id=?
        """, (org_id,)).fetchall()

    platform_log("SuperAdmin", "view_org", org["name"], f"Viewed org #{org_id} details")

    return jsonify({
        "org":           dict(org),
        "users":         [dict(r) for r in users],
        "extinguishers": [dict(r) for r in extinguishers],
        "assignments":   [dict(r) for r in assignments],
        "reports":       [dict(r) for r in reports],
        "companies":     [dict(r) for r in companies],
        "buildings":     [dict(r) for r in buildings],
    })

@app.route("/api/platform/audit", methods=["GET"])
def platform_audit_log():
    """Return the platform audit log with optional filters (action, date_from, date_to)."""
    limit       = request.args.get("limit", 200, type=int)
    action_f    = request.args.get("action", "").strip()
    date_from   = request.args.get("date_from", "").strip()
    date_to     = request.args.get("date_to", "").strip()

    conditions, params = [], []
    if action_f:
        conditions.append("action LIKE ?"); params.append(f"%{action_f}%")
    if date_from:
        conditions.append("timestamp >= ?"); params.append(date_from)
    if date_to:
        conditions.append("timestamp <= ?"); params.append(date_to + " 23:59:59")

    where = f"WHERE {' AND '.join(conditions)}" if conditions else ""
    params.append(limit)
    with get_db() as conn:
        rows = conn.execute(
            f"SELECT * FROM PlatformAuditLog {where} ORDER BY timestamp DESC LIMIT ?", params
        ).fetchall()
    return jsonify([dict(r) for r in rows])


@app.route("/api/platform/compliance", methods=["GET"])
def platform_compliance_stats():
    """Platform-wide and per-org compliance breakdown for the dashboard."""
    with get_db() as conn:
        rows = conn.execute("""
            SELECT
                o.org_id, o.name,
                (SELECT COUNT(*) FROM Extinguishers e
                 WHERE e.org_id = o.org_id
                   AND e.ext_status NOT IN ('Condemned/Removed','Retired')
                   AND (e.next_due_date IS NULL OR e.next_due_date >= date('now'))
                ) AS compliant,
                (SELECT COUNT(*) FROM Extinguishers e
                 WHERE e.org_id = o.org_id
                   AND e.ext_status NOT IN ('Condemned/Removed','Retired','Out of Service','Missing')
                   AND e.next_due_date IS NOT NULL AND e.next_due_date < date('now')
                ) AS overdue,
                (SELECT COUNT(*) FROM Extinguishers e
                 WHERE e.org_id = o.org_id
                   AND e.ext_status IN ('Out of Service','Missing')
                ) AS non_compliant,
                (SELECT COUNT(*) FROM Extinguishers e
                 WHERE e.org_id = o.org_id
                   AND e.ext_status NOT IN ('Condemned/Removed','Retired')
                ) AS active_total
            FROM Organizations o
            ORDER BY o.name
        """).fetchall()
    result = [dict(r) for r in rows]
    totals = {"compliant": 0, "overdue": 0, "non_compliant": 0, "active_total": 0}
    for r in result:
        for k in totals:
            totals[k] += r.get(k) or 0
    return jsonify({"orgs": result, "totals": totals})


# ── Platform Announcements ────────────────────────────────────────────────────

@app.route("/api/platform/announcements", methods=["GET"])
def platform_list_announcements():
    """List all announcements (Platform Admin view — all, including inactive)."""
    with get_db() as conn:
        rows = conn.execute(
            "SELECT * FROM PlatformAnnouncements ORDER BY created_at DESC"
        ).fetchall()
    return jsonify([dict(r) for r in rows])


@app.route("/api/platform/announcements", methods=["POST"])
def platform_create_announcement():
    """Create a new platform-wide announcement."""
    d = request.get_json()
    title      = (d.get("title")    or "").strip()
    body       = (d.get("body")     or "").strip()
    priority   = (d.get("priority") or "info").strip()
    expires_at = d.get("expires_at") or None
    if not title or not body:
        return jsonify({"error": "Title and body are required."}), 400
    with get_db() as conn:
        conn.execute(
            "INSERT INTO PlatformAnnouncements (title, body, priority, expires_at) VALUES (?,?,?,?)",
            (title, body, priority, expires_at)
        )
    platform_log("SuperAdmin", "announcement_created", title, f"Priority: {priority}")
    return jsonify({"success": True}), 201


@app.route("/api/platform/announcements/<int:ann_id>", methods=["PUT"])
def platform_update_announcement(ann_id):
    """Update an announcement (e.g. deactivate, change body)."""
    d = request.get_json()
    try:
        with get_db() as conn:
            updates, params = [], []
            for field in ["title", "body", "priority", "expires_at", "is_active"]:
                if field in d:
                    updates.append(f"{field}=?"); params.append(d[field])
            if not updates:
                return jsonify({"error": "No fields provided."}), 400
            params.append(ann_id)
            conn.execute(
                f"UPDATE PlatformAnnouncements SET {', '.join(updates)} WHERE announcement_id=?",
                params
            )
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/api/platform/announcements/<int:ann_id>", methods=["DELETE"])
def platform_delete_announcement(ann_id):
    """Permanently delete an announcement."""
    with get_db() as conn:
        row = conn.execute(
            "SELECT title FROM PlatformAnnouncements WHERE announcement_id=?", (ann_id,)
        ).fetchone()
        if not row:
            return jsonify({"error": "Announcement not found."}), 404
        conn.execute("DELETE FROM PlatformAnnouncements WHERE announcement_id=?", (ann_id,))
    platform_log("SuperAdmin", "announcement_deleted", row["title"], f"Deleted #{ann_id}")
    return jsonify({"success": True})


@app.route("/api/announcements", methods=["GET"])
def get_active_announcements():
    """Public endpoint — org users read active, non-expired announcements."""
    with get_db() as conn:
        rows = conn.execute("""
            SELECT announcement_id, title, body, priority, created_at, expires_at
            FROM PlatformAnnouncements
            WHERE is_active = 1
              AND (expires_at IS NULL OR expires_at > datetime('now'))
            ORDER BY created_at DESC
        """).fetchall()
    return jsonify([dict(r) for r in rows])


# ── REPORT COUNTER-SIGN (Admin approves a 3rd Party report) ──────────────────
@app.route("/api/reports/<int:report_id>/approve", methods=["POST"])
def approve_report(report_id):
    """Counter-sign a Pending_Review report submitted by a 3rd Party Inspector.
    Sets approval_status = Approved, records the reviewing admin and timestamp.
    Only Primary Admins can counter-sign — this is a legal attestation."""
    d = request.get_json() or {}
    admin_id = d.get("admin_id")
    if not admin_id:
        return jsonify({"error": "admin_id is required"}), 400
    try:
        with get_db() as conn:
            # Verify the report exists and is in Pending_Review
            report = conn.execute(
                "SELECT report_id, approval_status FROM Reports WHERE report_id=?", (report_id,)
            ).fetchone()
            if not report:
                return jsonify({"error": "Report not found"}), 404
            if report["approval_status"] == "Void":
                return jsonify({"error": "Cannot approve a voided report"}), 400
            conn.execute(
                """UPDATE Reports SET
                   approval_status='Approved',
                   reviewed_by=?,
                   reviewed_at=datetime('now')
                   WHERE report_id=?""",
                (admin_id, report_id)
            )
        platform_log("admin", "report_approved", str(report_id),
                     f"Counter-signed by admin_id={admin_id}")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── REPORT VOID (Admin voids a report with reason) ────────────────────────────
@app.route("/api/reports/<int:report_id>/void", methods=["POST"])
def void_report(report_id):
    """Void a report — marks it permanently invalid with a reason.
    Industry standard: reports cannot be deleted, only voided.
    The voided record remains in the audit trail.

    After voiding:
    1. The linked assignment reverts to 'Pending Inspection'
    2. The extinguisher's last_report_id rolls back to its previous valid report
    3. Returns ext and assignment data so the UI can prompt for reassignment."""
    d = request.get_json() or {}
    reason   = (d.get("reason") or "").strip()
    admin_id = d.get("admin_id")
    if not reason:
        return jsonify({"error": "A void reason is required"}), 400
    try:
        with get_db() as conn:
            # Get the report being voided — need ext_id and assignment link
            report = conn.execute(
                """SELECT r.report_id, r.extinguisher_id, r.inspector_id,
                          a.assignment_id, a.inspector_id AS assign_inspector_id,
                          e.building_id, e.floor_number, e.room_number,
                          e.location_description,
                          b.name AS building_name, c.name AS company_name
                   FROM Reports r
                   LEFT JOIN Assignments a ON a.extinguisher_id = r.extinguisher_id
                       AND a.inspector_id = r.inspector_id
                       AND a.status = 'Inspection Complete'
                   LEFT JOIN Extinguishers e ON e.extinguisher_id = r.extinguisher_id
                   LEFT JOIN Buildings b ON b.building_id = e.building_id
                   LEFT JOIN Companies c ON c.company_id = b.company_id
                   WHERE r.report_id = ?
                   ORDER BY a.assignment_id DESC LIMIT 1""",
                (report_id,)
            ).fetchone()

            if not report:
                return jsonify({"error": "Report not found"}), 404

            ext_id = report["extinguisher_id"]

            # 1. Void the report
            conn.execute(
                """UPDATE Reports SET
                   approval_status='Void',
                   void_reason=?,
                   reviewed_by=?,
                   reviewed_at=datetime('now')
                   WHERE report_id=?""",
                (reason, admin_id, report_id)
            )

            # 2. Revert linked assignment back to Pending Inspection
            assignment_id = report["assignment_id"]
            if assignment_id:
                conn.execute(
                    """UPDATE Assignments SET status='Pending Inspection'
                       WHERE assignment_id=?""",
                    (assignment_id,)
                )

            # 3. Roll back extinguisher's last_report_id to previous valid report
            prev_report = conn.execute(
                """SELECT report_id FROM Reports
                   WHERE extinguisher_id=? AND report_id != ?
                   AND (approval_status IS NULL OR approval_status != 'Void')
                   ORDER BY inspection_date DESC LIMIT 1""",
                (ext_id, report_id)
            ).fetchone()
            conn.execute(
                "UPDATE Extinguishers SET last_report_id=? WHERE extinguisher_id=?",
                (prev_report["report_id"] if prev_report else None, ext_id)
            )

            conn.commit()

        platform_log("admin", "report_voided", str(report_id),
                     f"Reason: {reason} | EXT-{ext_id} reverted to pending")

        return jsonify({
            "success": True,
            "extinguisher_id": ext_id,
            "assignment_id":   assignment_id,
            "ext_info": {
                "extinguisher_id":     ext_id,
                "building_name":       report["building_name"],
                "company_name":        report["company_name"],
                "floor_number":        report["floor_number"],
                "room_number":         report["room_number"],
                "location_description":report["location_description"],
            }
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── EXTINGUISHER CONDEMN & REPLACE ────────────────────────────────────────────
@app.route("/api/extinguishers/<int:ext_id>/condemn", methods=["POST"])
def condemn_extinguisher(ext_id):
    """Mark an extinguisher as Condemned/Removed (preserving full history),
    and create a new replacement extinguisher on the same wall hook.
    Returns the new extinguisher_id so the UI can open the edit modal.
    This is the legally correct workflow — never overwrite serial numbers."""
    d = request.get_json() or {}
    try:
        with get_db() as conn:
            # Get the old extinguisher to copy location data
            old = conn.execute(
                "SELECT * FROM Extinguishers WHERE extinguisher_id=?", (ext_id,)
            ).fetchone()
            if not old:
                return jsonify({"error": "Extinguisher not found"}), 404

            # Mark old as condemned — preserve all history
            conn.execute(
                """UPDATE Extinguishers SET
                   ext_status='Condemned/Removed',
                   condemned_date=date('now')
                   WHERE extinguisher_id=?""",
                (ext_id,)
            )

            # Create new replacement record — inherit location, interval, org
            # Serial number, DOT cert, manufacturer intentionally blank (new unit)
            conn.execute(
                """INSERT INTO Extinguishers
                   (address, building_number, floor_number, room_number,
                    location_description, inspection_interval_days,
                    building_id, org_id, ext_status, replaced_by_id)
                   VALUES (?,?,?,?,?,?,?,?,'Active',?)""",
                (old["address"], old["building_number"], old["floor_number"],
                 old["room_number"], old["location_description"],
                 old["inspection_interval_days"],
                 old["building_id"], old["org_id"], ext_id)
            )
            new_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]

            # Link old to new
            conn.execute(
                "UPDATE Extinguishers SET replaced_by_id=? WHERE extinguisher_id=?",
                (new_id, ext_id)
            )

        platform_log("inspector", "extinguisher_condemned", str(ext_id),
                     f"Replaced by new EXT-{new_id}")
        return jsonify({"success": True, "new_extinguisher_id": new_id}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── LOANER / SWAP TRACKING ────────────────────────────────────────────────────
@app.route("/api/extinguishers/<int:ext_id>/loaner", methods=["POST"])
def set_loaner(ext_id):
    """Mark an extinguisher as a loaner deployed at a site while another unit
    is being serviced.  Body: { loaner_for_id (optional), clear: bool }
    - clear=true  → removes loaner status from this unit
    - clear=false → marks this unit as a loaner (optionally linked to original)"""
    d        = request.get_json() or {}
    clearing = bool(d.get("clear", False))
    with get_db() as conn:
        ext = conn.execute(
            "SELECT * FROM Extinguishers WHERE extinguisher_id=?", (ext_id,)
        ).fetchone()
        if not ext:
            return jsonify({"error": "Extinguisher not found"}), 404
        if clearing:
            conn.execute(
                "UPDATE Extinguishers SET is_loaner=0, loaner_for_id=NULL WHERE extinguisher_id=?",
                (ext_id,)
            )
            platform_log("inspector", "loaner_cleared", str(ext_id), "Loaner status removed")
            return jsonify({"success": True, "is_loaner": False}), 200
        else:
            loaner_for_id = d.get("loaner_for_id")
            conn.execute(
                "UPDATE Extinguishers SET is_loaner=1, loaner_for_id=? WHERE extinguisher_id=?",
                (loaner_for_id, ext_id)
            )
            platform_log("inspector", "loaner_set", str(ext_id),
                         f"Loaner deployed for ext_id={loaner_for_id}")
            return jsonify({"success": True, "is_loaner": True}), 200


# ── EXTINGUISHER LOCATION HISTORY ────────────────────────────────────────────
@app.route("/api/extinguishers/<int:ext_id>/location-history", methods=["GET"])
def get_location_history(ext_id):
    """Return all recorded location moves for this extinguisher, newest first."""
    try:
        with get_db() as conn:
            rows = conn.execute("""
                SELECT history_id, changed_at, changed_by_name,
                       old_building_name, old_floor_number, old_room_number, old_location_desc,
                       new_building_name, new_floor_number, new_room_number, new_location_desc,
                       notes
                FROM ExtinguisherLocationHistory
                WHERE extinguisher_id=?
                ORDER BY changed_at DESC
            """, (ext_id,)).fetchall()
        return jsonify([dict(r) for r in rows])
    except Exception as e:
        return jsonify({"error": str(e)}), 500


# ── AHJ / FIRE MARSHAL AUDITOR TOKEN ─────────────────────────────────────────
@app.route("/api/auditor/tokens", methods=["POST"])
def create_auditor_token():
    """Generate a time-limited read-only auditor link for a specific building.
    The token gives a Fire Marshal or insurance auditor access to one building's
    approved reports and extinguisher list — nothing else.
    Tokens expire after 24 hours by default (configurable up to 72h)."""
    import secrets, datetime
    d         = request.get_json() or {}
    org_id    = d.get("org_id")
    building_id = d.get("building_id")
    admin_id  = d.get("admin_id")
    label     = (d.get("label") or "AHJ Auditor").strip()
    hours     = min(int(d.get("hours", 24)), 72)  # max 72 hours

    if not org_id or not building_id:
        return jsonify({"error": "org_id and building_id are required"}), 400

    token     = secrets.token_urlsafe(32)
    expires   = (datetime.datetime.utcnow() + datetime.timedelta(hours=hours)).strftime("%Y-%m-%d %H:%M:%S")

    try:
        with get_db() as conn:
            conn.execute(
                """INSERT INTO AuditorTokens
                   (token, org_id, building_id, created_by, expires_at, label)
                   VALUES (?,?,?,?,?,?)""",
                (token, org_id, building_id, admin_id, expires, label)
            )
        platform_log("admin", "auditor_token_created", label,
                     f"building_id={building_id}, expires={expires}")
        return jsonify({"success": True, "token": token, "expires_at": expires}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/auditor/<token>", methods=["GET"])
def auditor_access(token):
    """Validate an auditor token and return read-only data for the scoped building.
    Returns extinguishers, approved reports only, and building info.
    Returns 403 if the token is expired or invalid."""
    import datetime
    with get_db() as conn:
        tok = conn.execute(
            "SELECT * FROM AuditorTokens WHERE token=?", (token,)
        ).fetchone()
        if not tok:
            return jsonify({"error": "Invalid or expired auditor link"}), 403
        # Check expiry
        expires = datetime.datetime.strptime(tok["expires_at"], "%Y-%m-%d %H:%M:%S")
        if datetime.datetime.utcnow() > expires:
            return jsonify({"error": "This auditor link has expired"}), 403

        # Fetch scoped data — approved reports only, one building
        building = conn.execute(
            """SELECT b.*, c.name AS company_name, o.name AS org_name
               FROM Buildings b
               LEFT JOIN Companies c ON c.company_id=b.company_id
               LEFT JOIN Organizations o ON o.org_id=b.org_id
               WHERE b.building_id=?""",
            (tok["building_id"],)
        ).fetchone()

        exts = conn.execute(
            """SELECT e.*, b2.name AS building_name, c2.name AS company_name
               FROM Extinguishers e
               LEFT JOIN Buildings b2 ON b2.building_id=e.building_id
               LEFT JOIN Companies c2 ON c2.company_id=b2.company_id
               WHERE e.building_id=?
               ORDER BY e.floor_number, e.room_number""",
            (tok["building_id"],)
        ).fetchall()

        reports = conn.execute(
            """SELECT r.*, u.username AS inspector, u.first_name, u.last_name,
                      u.cert_number, u.subcontractor_company_name
               FROM Reports r
               LEFT JOIN Extinguishers e ON r.extinguisher_id=e.extinguisher_id
               LEFT JOIN Users u ON r.inspector_id=u.user_id
               WHERE e.building_id=?
                 AND (r.approval_status='Approved' OR r.approval_status IS NULL)
               ORDER BY r.inspection_date DESC""",
            (tok["building_id"],)
        ).fetchall()

    platform_log("auditor", "auditor_access", tok["label"],
                 f"token={token[:8]}... building_id={tok['building_id']}")
    return jsonify({
        "label":        tok["label"],
        "expires_at":   tok["expires_at"],
        "building":     dict(building) if building else {},
        "extinguishers":[dict(r) for r in exts],
        "reports":      [dict(r) for r in reports],
    })

@app.route("/api/auditor/tokens", methods=["GET"])
def list_auditor_tokens():
    """List all auditor tokens for an org (Admin only). Shows active + expired."""
    org_id = request.args.get("org_id")
    with get_db() as conn:
        rows = conn.execute(
            """SELECT t.*, b.name AS building_name, c.name AS company_name
               FROM AuditorTokens t
               LEFT JOIN Buildings b ON b.building_id=t.building_id
               LEFT JOIN Companies c ON c.company_id=b.company_id
               WHERE t.org_id=?
               ORDER BY t.created_at DESC""",
            (org_id,)
        ).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/auditor/tokens/<int:token_id>", methods=["DELETE"])
def revoke_auditor_token(token_id):
    """Revoke (delete) an auditor token immediately."""
    try:
        with get_db() as conn:
            conn.execute("DELETE FROM AuditorTokens WHERE token_id=?", (token_id,))
        platform_log("admin", "auditor_token_revoked", str(token_id), "Manually revoked")
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── CLIENT COMPLIANCE STATS ───────────────────────────────────────────────────
@app.route("/api/client/stats", methods=["GET"])
def client_stats():
    """Compliance dashboard stats for the Client role.
    Scoped strictly to their company_id.
    Returns: total extinguishers, compliant count, deficiencies, next inspection."""
    company_id = request.args.get("company_id")
    if not company_id:
        return jsonify({"error": "company_id required"}), 400

    with get_db() as conn:
        today = __import__('datetime').date.today().isoformat()

        exts = conn.execute(
            """SELECT e.extinguisher_id, e.ext_status, e.next_due_date,
                      e.last_inspection_date, e.floor_number, e.room_number,
                      e.location_description, b.name AS building_name,
                      e.last_report_id
               FROM Extinguishers e
               LEFT JOIN Buildings b ON b.building_id=e.building_id
               WHERE b.company_id=?
                 AND (e.ext_status IS NULL OR e.ext_status != 'Condemned/Removed')
               ORDER BY e.next_due_date""",
            (company_id,)
        ).fetchall()

        # Latest approved report per extinguisher
        reports = conn.execute(
            """SELECT r.extinguisher_id, r.inspection_result, r.inspection_date,
                      r.service_type, r.report_id
               FROM Reports r
               LEFT JOIN Extinguishers e ON r.extinguisher_id=e.extinguisher_id
               LEFT JOIN Buildings b ON e.building_id=b.building_id
               WHERE b.company_id=?
                 AND (r.approval_status='Approved' OR r.approval_status IS NULL)
               ORDER BY r.inspection_date DESC""",
            (company_id,)
        ).fetchall()

    # Build latest result per extinguisher
    latest = {}
    for r in reports:
        if r["extinguisher_id"] not in latest:
            latest[r["extinguisher_id"]] = dict(r)

    total = len(exts)
    compliant = 0
    deficiencies = []
    overdue = []

    for e in exts:
        eid = e["extinguisher_id"]
        rep = latest.get(eid)
        passed = rep and rep["inspection_result"] == "Pass"
        if passed:
            compliant += 1
        else:
            deficiencies.append({
                "extinguisher_id":    eid,
                "building_name":      e["building_name"],
                "floor":              e["floor_number"],
                "room":               e["room_number"],
                "location":           e["location_description"],
                "last_result":        rep["inspection_result"] if rep else "Never inspected",
                "last_inspection":    rep["inspection_date"]   if rep else None,
            })
        if e["next_due_date"] and e["next_due_date"] < today:
            overdue.append(eid)

    return jsonify({
        "total":           total,
        "compliant":       compliant,
        "non_compliant":   total - compliant,
        "overdue":         len(overdue),
        "compliance_pct":  round((compliant / total * 100), 1) if total > 0 else 100,
        "deficiencies":    deficiencies,
        "extinguishers":   [dict(e) for e in exts],
    })

# ── SERVICE REQUESTS ──────────────────────────────────────────────────────────

@app.route("/api/service_requests", methods=["GET"])
def get_service_requests():
    db = get_db()
    org_id     = request.args.get("org_id",     type=int)
    company_id = request.args.get("company_id", type=int)
    ext_id     = request.args.get("extinguisher_id", type=int)

    query = """
        SELECT sr.*,
               e.location_description, e.ext_status,
               c.name  AS company_name,
               o.name  AS org_name
        FROM ServiceRequests sr
        LEFT JOIN Extinguishers e ON e.extinguisher_id = sr.extinguisher_id
        LEFT JOIN Companies     c ON c.company_id = sr.company_id
        LEFT JOIN Organizations o ON o.org_id = sr.org_id
        WHERE 1=1
    """
    params = []
    if org_id:
        query += " AND sr.org_id = ?";     params.append(org_id)
    if company_id:
        query += " AND sr.company_id = ?"; params.append(company_id)
    if ext_id:
        query += " AND sr.extinguisher_id = ?"; params.append(ext_id)
    query += " ORDER BY sr.created_at DESC"

    rows = db.execute(query, params).fetchall()
    return jsonify([dict(r) for r in rows])


@app.route("/api/service_requests", methods=["POST"])
def create_service_request():
    db   = get_db()
    data = request.get_json()
    required = ["org_id", "company_id", "extinguisher_id", "requested_by", "request_type"]
    for f in required:
        if not data.get(f):
            return jsonify({"error": f"Missing required field: {f}"}), 400

    db.execute("""
        INSERT INTO ServiceRequests
            (org_id, company_id, extinguisher_id, requested_by, request_type,
             part_number, quantity, vendor, estimated_cost, notes, status, created_at, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,'Pending',datetime('now'),datetime('now'))
    """, (
        data["org_id"], data["company_id"], data["extinguisher_id"],
        data["requested_by"], data["request_type"],
        data.get("part_number", ""), data.get("quantity", 1),
        data.get("vendor", ""), data.get("estimated_cost", 0),
        data.get("notes", "")
    ))
    db.commit()
    req_id = db.execute("SELECT last_insert_rowid()").fetchone()[0]
    return jsonify({"request_id": req_id, "status": "Pending"}), 201


@app.route("/api/service_requests/<int:req_id>", methods=["PUT"])
def update_service_request(req_id):
    db   = get_db()
    data = request.get_json()
    allowed = ["status", "admin_notes", "request_type", "part_number",
               "quantity", "vendor", "estimated_cost", "notes"]
    fields  = {k: data[k] for k in allowed if k in data}
    if not fields:
        return jsonify({"error": "No updatable fields provided"}), 400

    fields["updated_at"] = "datetime('now')"
    set_clause = ", ".join(
        f"{k} = datetime('now')" if k == "updated_at" else f"{k} = ?"
        for k in fields
    )
    values = [v for k, v in fields.items() if k != "updated_at"]
    values.append(req_id)
    db.execute(f"UPDATE ServiceRequests SET {set_clause} WHERE request_id = ?", values)
    db.commit()
    return jsonify({"ok": True})


# ── START ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 50)
    print("  FireWatch Server")
    print(f"  Database : {DB_PATH}")
    print(f"  Frontend : {UI_DIR}")
    print("  Open     : http://localhost:5000")
    print("=" * 50)
    # debug=False in production, port from environment variable (Render sets PORT)
    port = int(os.environ.get("PORT", 5000))
    debug = os.environ.get("RENDER") is None  # debug only when running locally
    app.run(debug=debug, host="0.0.0.0", port=port)
