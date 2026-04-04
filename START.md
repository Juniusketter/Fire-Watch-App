# FireWatch — Quick Start Guide

## Prerequisites (one-time setup)

### 1. Install Python
- Download Python 3.10 or higher from https://python.org/downloads
- During installation, **check "Add Python to PATH"** before clicking Install
- Verify the install by opening Command Prompt and running:
  ```bash
  python --version
  ```

### 2. Install Git *(skip if already installed)*
- Download from https://git-scm.com/downloads and install with default settings
- Verify with:
  ```bash
  git --version
  ```

---

## Step 1 — Get the project from GitHub

**Option A — Clone with Git (recommended):**
```bash
git clone https://github.com/Juniusketter/Fire-Watch-App.git
```

**Option B — Download ZIP:**
1. Go to https://github.com/Juniusketter/Fire-Watch-App
2. Click the green **Code** button → **Download ZIP**
3. Extract the ZIP to your Desktop or preferred folder

---

## Step 2 — Navigate into the project folder

```bash
cd Fire-Watch-App
```

Confirm you are in the right place by running `dir` (Windows) or `ls` (Mac/Linux).
You should see `server.py` in the output.

---

## Step 3 — Install Flask (one-time only)

```bash
pip install flask
```

If that fails, try:
```bash
pip3 install flask
```

---

## Step 4 — Start the server

```bash
python server.py
```

You should see this in the terminal:
```
==================================================
  FireWatch Server
  Database : ...\src\database\FireWatch.db
  Frontend : ...\src\frontend\Fire-Watch-UI
  Open     : http://localhost:5000
==================================================
```

> **Keep this terminal open.** Closing it stops the server.

---

## Step 5 — Open the app in your browser

Go to:
```
http://localhost:5000
```

You should see the FireWatch login screen.

---

## Step 6 — Log in

Use any of the following test accounts:

| Username | Password | Role |
|---|---|---|
| AdminTest | password123 | Admin |
| InspectorTest | password123 | Inspector |
| ThirdAdminTest | password123 | 3rd Party Admin |
| ThirdInvTest | password123 | 3rd Party Inspector |

---

## Step 7 — Stop the server

When you are done, go back to the terminal and press:
```
Ctrl + C
```

---

## Adding new data to the database

1. Open `src/database/FireWatch.db` in **DB Browser for SQLite**
   (free download at https://sqlitebrowser.org)
2. Add rows to any table — Extinguishers, Users, Assignments, or Reports
3. Save and close DB Browser
4. Refresh the browser and log in — your new data appears automatically

No code changes needed. The server always reads live from the database.

---

## API Endpoints (for reference)

| Method | URL | Description |
|---|---|---|
| POST | `/api/login` | Authenticate user (SHA-256 hashed password) |
| GET | `/api/extinguishers` | List all extinguishers |
| POST | `/api/extinguishers` | Add a new extinguisher |
| PUT | `/api/extinguishers/<id>` | Edit an extinguisher |
| DELETE | `/api/extinguishers/<id>` | Delete extinguisher and linked records |
| GET | `/api/assignments` | List assignments (filter by `?user_id=`) |
| POST | `/api/assignments` | Create a new assignment |
| PUT | `/api/assignments/<id>` | Edit an existing assignment |
| GET | `/api/reports` | List reports (filter by `?user_id=`) |
| POST | `/api/reports` | Submit an inspection report |
| GET | `/api/users` | List all users (passwords never returned) |
| POST | `/api/users` | Create a new user |
| GET | `/api/stats` | Live counts for all four tables |

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `python` not recognized | Reinstall Python and check "Add Python to PATH" during setup |
| `pip install flask` fails | Try `pip3 install flask` instead |
| Port 5000 already in use | Another instance of server.py is running — close it first |
| Login says "Invalid username or password" | Make sure you pulled the latest version from GitHub |
| Page won't load at localhost:5000 | Check the terminal — the server must still be running |
| Database errors on startup | Make sure you are running `python server.py` from the project root, not a subfolder |
| 600+ files showing in GitHub Desktop | The Qt build folder is being tracked — ensure `.gitignore` includes `src/frontend/Fire-Watch-UI/build/` |
