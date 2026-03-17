"""
FireWatch Backend Server
========================
Run this file to start the local backend server.

Requirements:
    pip install flask

Usage:
    python server.py

Then open your browser to:
    http://localhost:5000

The server reads/writes directly from src/database/FireWatch.db
Any changes you make to the database will appear live after login.
"""

from flask import Flask, request, jsonify, send_from_directory
import sqlite3
import os
import hashlib
import logging
import uuid

# Silence per-request logs — errors still print, routine GET/POST lines do not
logging.getLogger("werkzeug").setLevel(logging.ERROR)

def hash_password(pw: str) -> str:
    """SHA-256 hash a password. Matches Qt's QCryptographicHash::Sha256."""
    return hashlib.sha256(pw.encode()).hexdigest()

app = Flask(__name__)

# ── Paths ──────────────────────────────────────────────────────────────────────
BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
DB_PATH     = os.path.join(BASE_DIR, "src", "database", "FireWatch.db")
UI_DIR      = os.path.join(BASE_DIR, "src", "frontend", "Fire-Watch-UI")
UPLOAD_DIR  = os.path.join(BASE_DIR, "src", "uploads")
os.makedirs(UPLOAD_DIR, exist_ok=True)  # create uploads folder if it doesn't exist

ALLOWED_EXTENSIONS = {"jpg", "jpeg", "png", "gif", "webp", "heic", "heif"}

# ── DB helper ──────────────────────────────────────────────────────────────────
def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# ── Startup migrations — run once on server start ──────────────────────────────
def run_migrations():
    with get_db() as conn:

        # ── Users ──────────────────────────────────────────────────────────
        user_cols = [r[1] for r in conn.execute("PRAGMA table_info(Users)").fetchall()]
        if "preferences" not in user_cols:
            conn.execute("ALTER TABLE Users ADD COLUMN preferences TEXT DEFAULT '{}'")

        # ── Assignments ────────────────────────────────────────────────────
        assign_cols = [r[1] for r in conn.execute("PRAGMA table_info(Assignments)").fetchall()]
        if "due_date" not in assign_cols:
            conn.execute("ALTER TABLE Assignments ADD COLUMN due_date DATE")
        if "notes" not in assign_cols:
            conn.execute("ALTER TABLE Assignments ADD COLUMN notes TEXT DEFAULT ''")

        # ── Reports ────────────────────────────────────────────────────────
        report_cols = [r[1] for r in conn.execute("PRAGMA table_info(Reports)").fetchall()]
        if "photo_path" not in report_cols:
            conn.execute("ALTER TABLE Reports ADD COLUMN photo_path TEXT")

        # ── Companies table ────────────────────────────────────────────────
        tables = [r[0] for r in conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table'").fetchall()]

        if "Companies" not in tables:
            conn.execute("""
                CREATE TABLE Companies (
                    company_id   INTEGER PRIMARY KEY AUTOINCREMENT,
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

        # ── Buildings table ────────────────────────────────────────────────
        if "Buildings" not in tables:
            conn.execute("""
                CREATE TABLE Buildings (
                    building_id  INTEGER PRIMARY KEY AUTOINCREMENT,
                    company_id   INTEGER NOT NULL REFERENCES Companies(company_id),
                    name         TEXT NOT NULL,
                    address      TEXT,
                    floors       INTEGER DEFAULT 1,
                    notes        TEXT,
                    created_at   TEXT DEFAULT (date('now'))
                )
            """)

        # ── Add building_id to Extinguishers (nullable — safe for existing rows) ──
        ext_cols = [r[1] for r in conn.execute("PRAGMA table_info(Extinguishers)").fetchall()]
        if "building_id" not in ext_cols:
            conn.execute("ALTER TABLE Extinguishers ADD COLUMN building_id INTEGER REFERENCES Buildings(building_id)")

        conn.commit()

        # ── Auto-migrate existing extinguisher data into Companies/Buildings ──
        # Only runs if Companies table is empty (first time) and Extinguishers exist
        co_count  = conn.execute("SELECT COUNT(*) FROM Companies").fetchone()[0]
        ext_count = conn.execute("SELECT COUNT(*) FROM Extinguishers").fetchone()[0]

        if co_count == 0 and ext_count > 0:
            # Collect unique addresses from existing extinguishers
            rows = conn.execute(
                "SELECT extinguisher_id, address, building_number FROM Extinguishers"
            ).fetchall()

            # Group by address → one Company per unique address
            address_to_company = {}
            for row in rows:
                addr = row["address"] or "Unknown Address"
                if addr not in address_to_company:
                    # Derive a company name from the address
                    company_name = addr
                    conn.execute(
                        "INSERT INTO Companies (name, address) VALUES (?, ?)",
                        (company_name, addr)
                    )
                    conn.commit()
                    co_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]
                    address_to_company[addr] = co_id

            # One Building per unique (address + building_number) combo
            addr_bldg_to_building = {}
            for row in rows:
                addr     = row["address"] or "Unknown Address"
                bldg_num = row["building_number"] or "Main Building"
                key      = (addr, bldg_num)
                if key not in addr_bldg_to_building:
                    co_id = address_to_company[addr]
                    conn.execute(
                        "INSERT INTO Buildings (company_id, name, address) VALUES (?, ?, ?)",
                        (co_id, bldg_num, addr)
                    )
                    conn.commit()
                    bldg_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]
                    addr_bldg_to_building[key] = bldg_id

            # Link each extinguisher to its building
            for row in rows:
                addr     = row["address"] or "Unknown Address"
                bldg_num = row["building_number"] or "Main Building"
                bldg_id  = addr_bldg_to_building[(addr, bldg_num)]
                conn.execute(
                    "UPDATE Extinguishers SET building_id=? WHERE extinguisher_id=?",
                    (bldg_id, row["extinguisher_id"])
                )
            conn.commit()
            print(f"  [Migration] Auto-created {len(address_to_company)} companies, "
                  f"{len(addr_bldg_to_building)} buildings from existing extinguisher data.")

run_migrations()

# ── Serve frontend ─────────────────────────────────────────────────────────────
@app.route("/")
def index():
    return send_from_directory(UI_DIR, "index.html")

@app.route("/<path:filename>")
def static_files(filename):
    return send_from_directory(UI_DIR, filename)

@app.route("/uploads/<path:filename>")
def serve_upload(filename):
    return send_from_directory(UPLOAD_DIR, filename)

# ── PHOTO UPLOAD ───────────────────────────────────────────────────────────────
@app.route("/api/upload", methods=["POST"])
def upload_photo():
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
@app.route("/api/login", methods=["POST"])
def login():
    data     = request.get_json()
    username = (data.get("username") or "").strip()
    password = data.get("password") or ""

    if not username or not password:
        return jsonify({"success": False, "error": "Missing credentials"}), 400

    # Password arrives pre-hashed from client (SHA-256)
    with get_db() as conn:
        row = conn.execute(
            "SELECT user_id, username, role FROM Users "
            "WHERE username = ? AND password_hash = ?",
            (username, password)
        ).fetchone()

    if row:
        return jsonify({
            "success":  True,
            "user_id":  row["user_id"],
            "username": row["username"],
            "role":     row["role"]
        })
    return jsonify({"success": False, "error": "Invalid username or password"}), 401

# ── COMPANIES ──────────────────────────────────────────────────────────────────
@app.route("/api/companies", methods=["GET"])
def get_companies():
    with get_db() as conn:
        rows = conn.execute("""
            SELECT c.company_id, c.name, c.address, c.city, c.state, c.phone, c.contact_name,
                   COUNT(DISTINCT b.building_id) AS building_count,
                   COUNT(DISTINCT e.extinguisher_id) AS extinguisher_count
            FROM Companies c
            LEFT JOIN Buildings b ON b.company_id = c.company_id
            LEFT JOIN Extinguishers e ON e.building_id = b.building_id
            GROUP BY c.company_id
            ORDER BY c.name
        """).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/companies", methods=["POST"])
def add_company():
    d = request.get_json()
    name = (d.get("name") or "").strip()
    if not name:
        return jsonify({"error": "Company name is required."}), 400
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Companies (name, address, city, state, zip, phone, contact_name) "
                "VALUES (?,?,?,?,?,?,?)",
                (name, d.get("address",""), d.get("city",""), d.get("state",""),
                 d.get("zip",""), d.get("phone",""), d.get("contact_name",""))
            )
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
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/companies/<int:company_id>", methods=["DELETE"])
def delete_company(company_id):
    try:
        with get_db() as conn:
            # Get all buildings for this company
            bldgs = conn.execute(
                "SELECT building_id FROM Buildings WHERE company_id=?", (company_id,)
            ).fetchall()
            for b in bldgs:
                bid = b["building_id"]
                # Delete reports and assignments for extinguishers in this building
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
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── BUILDINGS ──────────────────────────────────────────────────────────────────
@app.route("/api/buildings", methods=["GET"])
def get_buildings():
    company_id = request.args.get("company_id")
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
                GROUP BY b.building_id
                ORDER BY b.name
            """, (company_id,)).fetchall()
        else:
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
                GROUP BY b.building_id
                ORDER BY c.name, b.name
            """).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/buildings", methods=["POST"])
def add_building():
    d = request.get_json()
    name       = (d.get("name") or "").strip()
    company_id = d.get("company_id")
    if not name or not company_id:
        return jsonify({"error": "Building name and company_id are required."}), 400
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Buildings (company_id, name, address, floors, notes) VALUES (?,?,?,?,?)",
                (company_id, name, d.get("address",""), d.get("floors", 1), d.get("notes",""))
            )
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
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── EXTINGUISHERS ──────────────────────────────────────────────────────────────
@app.route("/api/extinguishers", methods=["GET"])
def get_extinguishers():
    building_id = request.args.get("building_id")
    company_id  = request.args.get("company_id")
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
        if building_id:
            rows = conn.execute(base + " WHERE e.building_id = ? ORDER BY e.floor_number, e.room_number",
                                (building_id,)).fetchall()
        elif company_id:
            rows = conn.execute(base + " WHERE c.company_id = ? ORDER BY b.name, e.floor_number, e.room_number",
                                (company_id,)).fetchall()
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
                " location_description, inspection_interval_days, next_due_date, building_id) "
                "VALUES (?,?,?,?,?,?,?,?)",
                (d.get("address"), d.get("building_number"),
                 d.get("floor_number", 1), d.get("room_number"),
                 d.get("location_description"),
                 d.get("inspection_interval_days", 30),
                 d.get("next_due_date"),
                 d.get("building_id"))
            )
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/extinguishers/<int:ext_id>", methods=["PUT"])
def update_extinguisher(ext_id):
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "UPDATE Extinguishers SET address=?, building_number=?, "
                "floor_number=?, room_number=?, location_description=?, "
                "inspection_interval_days=?, next_due_date=?, building_id=? "
                "WHERE extinguisher_id=?",
                (d.get("address"), d.get("building_number"),
                 d.get("floor_number", 1), d.get("room_number"),
                 d.get("location_description"),
                 d.get("inspection_interval_days", 30),
                 d.get("next_due_date"),
                 d.get("building_id"),
                 ext_id)
            )
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
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── ASSIGNMENTS ────────────────────────────────────────────────────────────────
@app.route("/api/assignments", methods=["GET"])
def get_assignments():
    user_id = request.args.get("user_id")
    with get_db() as conn:
        # Ensure due_date and notes columns exist (migration guard)
        existing = [r[1] for r in conn.execute("PRAGMA table_info(Assignments)").fetchall()]
        if "due_date" not in existing:
            conn.execute("ALTER TABLE Assignments ADD COLUMN due_date DATE")
        if "notes" not in existing:
            conn.execute("ALTER TABLE Assignments ADD COLUMN notes TEXT DEFAULT ''")

        if user_id:
            rows = conn.execute(
                "SELECT a.assignment_id, u_a.username AS assigned_by, "
                "a.inspector_id, a.extinguisher_id, a.due_date, a.status, a.notes "
                "FROM Assignments a "
                "LEFT JOIN Users u_a ON a.admin_id = u_a.user_id "
                "WHERE a.inspector_id = ?", (user_id,)
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT a.assignment_id, u_a.username AS assigned_by, "
                "u_i.username AS inspector, a.inspector_id, "
                "a.extinguisher_id, a.due_date, a.status, a.notes "
                "FROM Assignments a "
                "LEFT JOIN Users u_a ON a.admin_id     = u_a.user_id "
                "LEFT JOIN Users u_i ON a.inspector_id = u_i.user_id"
            ).fetchall()
    return jsonify([dict(r) for r in rows])

# ── CREATE ASSIGNMENT ─────────────────────────────────────────────────────────
@app.route("/api/assignments", methods=["POST"])
def create_assignment():
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Assignments (admin_id, inspector_id, extinguisher_id, due_date, notes, status) "
                "VALUES (?, ?, ?, ?, ?, 'Pending Inspection')",
                (d.get("admin_id"), d.get("inspector_id"), d.get("extinguisher_id"),
                 d.get("due_date", ""), d.get("notes", ""))
            )
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
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── REPORTS ────────────────────────────────────────────────────────────────────
@app.route("/api/reports", methods=["GET"])
def get_reports():
    user_id = request.args.get("user_id")
    with get_db() as conn:
        if user_id:
            rows = conn.execute(
                "SELECT report_id, extinguisher_id, inspection_date, notes, photo_path "
                "FROM Reports WHERE inspector_id = ?", (user_id,)
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT r.report_id, r.extinguisher_id, "
                "u.username AS inspector, r.inspection_date, r.notes, r.photo_path "
                "FROM Reports r LEFT JOIN Users u ON r.inspector_id = u.user_id"
            ).fetchall()
    return jsonify([dict(r) for r in rows])

# ── CREATE REPORT ─────────────────────────────────────────────────────────────
@app.route("/api/reports", methods=["POST"])
def create_report():
    d = request.get_json()
    try:
        with get_db() as conn:
            # Ensure photo_path column exists
            report_cols = [r[1] for r in conn.execute("PRAGMA table_info(Reports)").fetchall()]
            if "photo_path" not in report_cols:
                conn.execute("ALTER TABLE Reports ADD COLUMN photo_path TEXT")
            conn.execute(
                "INSERT INTO Reports (extinguisher_id, inspector_id, inspection_date, notes, photo_path) "
                "VALUES (?, ?, ?, ?, ?)",
                (d.get("extinguisher_id"), d.get("inspector_id"),
                 d.get("inspection_date"), d.get("notes", ""),
                 d.get("photo_path"))
            )
            # Mark assignment complete
            if d.get("assignment_id"):
                conn.execute(
                    "UPDATE Assignments SET status = 'Inspection Complete' "
                    "WHERE assignment_id = ?",
                    (d.get("assignment_id"),)
                )
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── USERS ──────────────────────────────────────────────────────────────────────
@app.route("/api/users", methods=["GET"])
def get_users():
    with get_db() as conn:
        rows = conn.execute(
            "SELECT user_id, username, role FROM Users"
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
    d = request.get_json()
    username = (d.get("username") or "").strip()
    password = (d.get("password") or "").strip()
    role     = (d.get("role") or "").strip()

    if not username or not password or not role:
        return jsonify({"error": "Username, password, and role are required."}), 400

    valid_roles = ["Admin", "Inspector", "3rd_Party_Admin", "3rd_Party_Inspector"]
    if role not in valid_roles:
        return jsonify({"error": f"Invalid role. Must be one of: {', '.join(valid_roles)}"}), 400

    # Password arrives pre-hashed from client (SHA-256), store as-is
    try:
        with get_db() as conn:
            # Check for duplicate username
            existing = conn.execute(
                "SELECT user_id FROM Users WHERE username = ?", (username,)
            ).fetchone()
            if existing:
                return jsonify({"error": "Username already exists."}), 409
            conn.execute(
                "INSERT INTO Users (username, password_hash, role) VALUES (?, ?, ?)",
                (username, password, role)
            )
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── NOTIFICATIONS ─────────────────────────────────────────────────────────────
@app.route("/api/notifications", methods=["GET"])
def get_notifications():
    from datetime import datetime, timedelta
    user_id = request.args.get("user_id")
    role    = request.args.get("role", "")
    since   = (datetime.now() - timedelta(hours=24)).strftime("%Y-%m-%d %H:%M:%S")
    notifs  = []

    with get_db() as conn:
        # ── New assignments ────────────────────────────────────────────────
        if role in ("Admin", "3rd_Party_Admin"):
            rows = conn.execute(
                "SELECT a.assignment_id, u_i.username AS inspector, a.extinguisher_id, a.due_date "
                "FROM Assignments a LEFT JOIN Users u_i ON a.inspector_id = u_i.user_id "
                "ORDER BY a.assignment_id DESC LIMIT 10"
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT a.assignment_id, u_a.username AS admin, a.extinguisher_id, a.due_date "
                "FROM Assignments a LEFT JOIN Users u_a ON a.admin_id = u_a.user_id "
                "WHERE a.inspector_id = ? ORDER BY a.assignment_id DESC LIMIT 10", (user_id,)
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
                "WHERE a.status = 'Inspection Complete' ORDER BY a.assignment_id DESC LIMIT 5"
            ).fetchall()
        else:
            completed = conn.execute(
                "SELECT a.assignment_id, a.extinguisher_id FROM Assignments a "
                "WHERE a.status = 'Inspection Complete' AND a.inspector_id = ? "
                "ORDER BY a.assignment_id DESC LIMIT 5", (user_id,)
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
                "ORDER BY r.report_id DESC LIMIT 10"
            ).fetchall()
        else:
            reports = conn.execute(
                "SELECT r.report_id, r.extinguisher_id, r.inspection_date FROM Reports r "
                "WHERE r.inspector_id = ? ORDER BY r.report_id DESC LIMIT 10", (user_id,)
            ).fetchall()
        for r in reports:
            notifs.append({"id": f"report_{r['report_id']}", "type": "report_submitted",
                           "message": f"Inspection report #{r['report_id']} submitted for Ext #{r['extinguisher_id']}",
                           "timestamp": r['inspection_date'] or since})

        # ── Overdue extinguishers ──────────────────────────────────────────
        today = datetime.now().strftime("%Y-%m-%d")
        overdue = conn.execute(
            "SELECT extinguisher_id, address, next_due_date FROM Extinguishers "
            "WHERE next_due_date IS NOT NULL AND next_due_date < ? "
            "ORDER BY next_due_date ASC LIMIT 10", (today,)
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
    with get_db() as conn:
        ext     = conn.execute("SELECT COUNT(*) FROM Extinguishers").fetchone()[0]
        assigns = conn.execute("SELECT COUNT(*) FROM Assignments").fetchone()[0]
        reports = conn.execute("SELECT COUNT(*) FROM Reports").fetchone()[0]
        users   = conn.execute("SELECT COUNT(*) FROM Users").fetchone()[0]
    return jsonify({
        "extinguishers": ext,
        "assignments":   assigns,
        "reports":       reports,
        "users":         users
    })

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
