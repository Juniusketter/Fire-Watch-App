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

app = Flask(__name__)

# ── Paths ──────────────────────────────────────────────────────────────────
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH  = os.path.join(BASE_DIR, "src", "database", "FireWatch.db")
UI_DIR   = os.path.join(BASE_DIR, "src", "frontend", "Fire-Watch-UI")

# ── DB helper ──────────────────────────────────────────────────────────────
def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# ── Serve the frontend ─────────────────────────────────────────────────────
@app.route("/")
def index():
    return send_from_directory(UI_DIR, "index.html")

@app.route("/<path:filename>")
def static_files(filename):
    return send_from_directory(UI_DIR, filename)

# ── AUTH ───────────────────────────────────────────────────────────────────
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
            "SELECT user_id, username, role FROM Users WHERE username = ? AND password_hash = ?",
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

# ── EXTINGUISHERS ──────────────────────────────────────────────────────────
@app.route("/api/extinguishers", methods=["GET"])
def get_extinguishers():
    with get_db() as conn:
        rows = conn.execute("SELECT * FROM Extinguishers").fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/extinguishers", methods=["POST"])
def add_extinguisher():
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Extinguishers "
                "(address, building_number, floor_number, room_number, "
                " location_description, inspection_interval_days, next_due_date) "
                "VALUES (?,?,?,?,?,?,?)",
                (d.get("address"), d.get("building_number"),
                 d.get("floor_number", 1), d.get("room_number"),
                 d.get("location_description"),
                 d.get("inspection_interval_days", 30),
                 d.get("next_due_date"))
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
                "inspection_interval_days=?, next_due_date=? "
                "WHERE extinguisher_id=?",
                (d.get("address"), d.get("building_number"),
                 d.get("floor_number", 1), d.get("room_number"),
                 d.get("location_description"),
                 d.get("inspection_interval_days", 30),
                 d.get("next_due_date"), ext_id)
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

# ── ASSIGNMENTS ────────────────────────────────────────────────────────────
@app.route("/api/assignments", methods=["GET"])
def get_assignments():
    user_id = request.args.get("user_id")
    with get_db() as conn:
        # Migration guard — add columns if missing
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

# ── REPORTS ────────────────────────────────────────────────────────────────
@app.route("/api/reports", methods=["GET"])
def get_reports():
    user_id = request.args.get("user_id")
    with get_db() as conn:
        if user_id:
            rows = conn.execute(
                "SELECT report_id, extinguisher_id, inspection_date, notes "
                "FROM Reports WHERE inspector_id = ?", (user_id,)
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT r.report_id, r.extinguisher_id, "
                "u.username AS inspector, r.inspection_date, r.notes "
                "FROM Reports r LEFT JOIN Users u ON r.inspector_id = u.user_id"
            ).fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/reports", methods=["POST"])
def create_report():
    d = request.get_json()
    try:
        with get_db() as conn:
            conn.execute(
                "INSERT INTO Reports (extinguisher_id, inspector_id, inspection_date, notes) "
                "VALUES (?, ?, ?, ?)",
                (d.get("extinguisher_id"), d.get("inspector_id"),
                 d.get("inspection_date"), d.get("notes", ""))
            )
            if d.get("assignment_id"):
                conn.execute(
                    "UPDATE Assignments SET status = 'Inspection Complete' "
                    "WHERE assignment_id = ?",
                    (d.get("assignment_id"),)
                )
        return jsonify({"success": True}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ── USERS ──────────────────────────────────────────────────────────────────
@app.route("/api/users", methods=["GET"])
def get_users():
    with get_db() as conn:
        rows = conn.execute("SELECT user_id, username, role FROM Users").fetchall()
    return jsonify([dict(r) for r in rows])

@app.route("/api/users", methods=["POST"])
def create_user():
    d = request.get_json()
    username = (d.get("username") or "").strip()
    password = (d.get("password") or "").strip()
    role     = (d.get("role") or "").strip()

    if not username or not password or not role:
        return jsonify({"error": "Username, password, and role are required."}), 400

    try:
        with get_db() as conn:
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

# ── SUMMARY STATS ──────────────────────────────────────────────────────────
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

# ── START ──────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 50)
    print("  FireWatch Server")
    print(f"  Database : {DB_PATH}")
    print(f"  Frontend : {UI_DIR}")
    print("  Open     : http://localhost:5000")
    print("=" * 50)
    app.run(debug=True, port=5000)
