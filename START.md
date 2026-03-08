# FireWatch — Quick Start Guide

## Requirements
- Python 3.8 or higher (check with `python3 --version`)
- pip (comes with Python)

---

## Step 1 — Install Flask (one time only)

Open a terminal in this folder and run:

```bash
pip install flask
```

Or if you use pip3:

```bash
pip3 install flask
```

---

## Step 2 — Start the server

From the root of this project (where `server.py` lives), run:

```bash
python server.py
```

You should see:
```
==================================================
  FireWatch Server
  Database : .../src/database/FireWatch.db
  Frontend : .../src/frontend/Fire-Watch-UI
  Open     : http://localhost:5000
==================================================
```

---

## Step 3 — Open the app

Open your browser and go to:

```
http://localhost:5000
```

Log in with:
- **Username:** `AdminTest`
- **Password:** `password123`

---

## Adding new data to the database

1. Open `src/database/FireWatch.db` in **DB Browser for SQLite** (free download at sqlitebrowser.org)
2. Add rows to any table — Extinguishers, Users, Assignments, Reports, or new tables you create
3. Save and close DB Browser
4. Refresh the browser at `http://localhost:5000` and log in — your new data appears automatically

No code changes needed. The server always reads live from the database.

---

## API Endpoints (for reference)

| Method | URL                   | Description                        |
|--------|-----------------------|------------------------------------|
| POST   | `/api/login`          | Login with username + password     |
| GET    | `/api/extinguishers`  | All extinguishers                  |
| GET    | `/api/assignments`    | All assignments                    |
| GET    | `/api/reports`        | All reports                        |
| GET    | `/api/users`          | All users (no passwords returned)  |
| GET    | `/api/stats`          | Row counts for all tables          |

---

## Stopping the server

Press `Ctrl + C` in the terminal.
