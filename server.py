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
    conn.row_factory = sqlite3.Row          # rows behave like dicts
    return conn

# ── Serve the frontend ─────────────────────────────────────────────────────
@app.route("/")
def index():
    return send_from_directory(UI_DIR, "index.html")

# ── AUTH ───────────────────────────────────────────────────────────────────
@app.route("/api/login", methods=["POST"])
def login():
    data     = request.get_json()
    username = (data.get("username") or "").strip()
    password = data.get("password") or ""

    if not username or not password:
        return jsonify({"success": False, "error": "Missing credentials"}), 400

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

# ── ASSIGNMENTS ────────────────────────────────────────────────────────────
@app.route("/api/assignments", methods=["GET"])
def get_assignments():
    with get_db() as conn:
        rows = conn.execute("SELECT * FROM Assignments").fetchall()
    return jsonify([dict(r) for r in rows])

# ── REPORTS ────────────────────────────────────────────────────────────────
@app.route("/api/reports", methods=["GET"])
def get_reports():
    with get_db() as conn:
        rows = conn.execute("SELECT * FROM Reports").fetchall()
    return jsonify([dict(r) for r in rows])

# ── USERS (Admin only) ─────────────────────────────────────────────────────
@app.route("/api/users", methods=["GET"])
def get_users():
    with get_db() as conn:
        # Never return password hashes to the frontend
        rows = conn.execute("SELECT user_id, username, role FROM Users").fetchall()
    return jsonify([dict(r) for r in rows])

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
