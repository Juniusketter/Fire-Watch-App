# 🔥 FireWatch — NFPA 10 Fire Extinguisher Compliance Tracking

**FireWatch** is a full-stack multi-tenant SaaS platform for managing NFPA 10 fire extinguisher compliance across organizations, buildings, and inspection teams.

> **CSC480C — Computer Science Capstone Project III**
> Professor Mila Hose · National University, San Diego
> Team: Fire Wardens Safety

---

## 🌐 Live Demo

**Web Dashboard:** https://fire-watch-app.onrender.com
**GitHub:** https://github.com/Juniusketter/Fire-Watch-App

> ⚠️ Render free tier may cold-start — allow 30–60 seconds on first load.

---

## 👥 Team

| Name | Role |
|------|------|
| Junius Ketter | Lead Developer — Database & Frontend |
| Lillian Bowen | PM / Backend Lead |
| Rafael G. Santiago Montalvo | Frontend Developer |
| Quinci Walker | Backend Developer / QA |

---

## 🛠 Tech Stack

| Layer | Technology |
|-------|-----------|
| Web Backend | Python Flask · 41 REST endpoints · SHA-256 authentication |
| Web Frontend | Vanilla HTML / CSS / JavaScript (single-file SPA) |
| Desktop App | Qt Creator 6.10.2 · C++ · MinGW 64-bit |
| Database | SQLite — 9-table schema |
| Security | SHA-256 client-side hashing (QCryptographicHash + Web Crypto API) |
| Deployment | Render.com · Auto-deploy via GitHub push to main |

---

## 🔐 Six-Role Architecture

| Role | Access Scope |
|------|-------------|
| **Platform Admin** | Full cross-org control — all organizations, orgs approval/suspend |
| **Admin** | Full org — extinguishers, assignments, reports, users, Priority Queue |
| **Inspector** | Building-scoped — flat extinguisher view, assignment completion, photo upload |
| **3rd Party Admin** | Team-scoped — manage their assigned sub-inspector team only |
| **3rd Party Inspector** | Unit-scoped — their specific assigned extinguisher only |
| **Client** | Company drill-down read-only — extinguisher status, QR Code, NFPA Form |

---

## 🗄️ Database Schema (9 Tables)

`Organizations` · `Buildings` · `Extinguishers` · `Users` · `Assignments` · `Reports` · `Announcements` · `Messages` · `ServiceRequests`

---

## ✅ Feature Set (All Sprints)

### Core Platform
- Multi-tenant organization architecture with Platform Admin cross-org control
- Account approval system — new accounts start pending, approved by org Admin
- SHA-256 authentication, 15-minute auto-logout with inactivity warning modal
- Stay-logged-in toggle, session persistence
- 4 UI themes (Dark, Light, Navy, High Contrast + Red, Orange, Green, Slate, White)
- **English / Spanish language toggle** — 490+ translation pairs, persists to localStorage
- Platform-wide announcement banners with priority colors

### Extinguisher Management
- Full CRUD with all NFPA 10 metadata fields (type, class, size, manufacturer, serial, DOT cert, hydrostatic dates)
- Priority Queue — scoring algorithm ranks extinguishers by inspection urgency
- Bulk CSV import with column mapping
- QR Code generation per extinguisher — scan → direct detail view (mobile-safe)
- Condemnation + replacement workflow (preserves inspection history)

### Inspection Workflow
- Assignment creation by building, inspector, or extinguisher
- Bulk assignment generation
- NFPA 10 §7.2.2 compliance checklist embedded in inspection form
- Photo upload on inspection reports
- Monthly Inspection Calendar (Admin view)
- Report history per extinguisher
- Void Report workflow — mandatory reason, audit trail preserved, VOID badge

### Reporting & Export
- PDF Report Generator — professional inspection reports per record
- NFPA Form Generator — printable NFPA-compliant extinguisher tags
- Bulk NFPA Form generation (filter by building/result/date)
- CSV / PDF / Word export
- Annual compliance report (Client role)
- Shareable read-only Auditor Link (time-limited, building-scoped)

### Service Requests
- Clients submit service requests for extinguishers with deficiencies
- Admin updates status and adds notes
- "Service Requested" state reflects on Deficiencies tab after submission (optimistic update)
- Scoped by org_id / company_id per role

### Messaging & Notifications
- In-app messaging (Direct Message, Group Send, Broadcast)
- Email composition and send
- Notification feed with mark-all-read

### Qt Desktop App
- C++ Qt desktop sync with Flask REST API
- org_id-scoped tabs, NFPA metadata columns, Edit/Delete user dialogs
- Consolidated Due Date Summary dialog

---

## 🧪 Test Accounts

All test accounts use password: `password123`

| Username | Role |
|----------|------|
| `AdminTest` | Admin (org 1 — Fire Wardens Safety) |
| `InspectorTest` | Inspector (org 1) |
| `ThirdAdminTest` | 3rd Party Admin (org 1) |
| `ThirdInvTest` | 3rd Party Inspector (org 1) |
| `ClientTest` | Client (org 1) |

Platform Admin password: `FireWatch2026!` (enter on the Platform Admin login screen)

---

## 📁 Project Structure

```
Fire-Watch-App/
├── server.py                        # Flask backend — 41 REST endpoints
├── landing.html                     # Public landing page
├── README.md
├── src/
│   ├── database/
│   │   └── FireWatch.db             # SQLite database
│   ├── frontend/
│   │   └── Fire-Watch-UI/
│   │       └── index.html           # Full SPA — HTML/CSS/JS (~14,000+ lines)
│   ├── backend/                     # Qt/C++ source files
│   │   ├── main.cpp
│   │   ├── User.cpp / User.h
│   │   ├── Admin.cpp / Admin.h
│   │   ├── dashboard.cpp / dashboard.h
│   │   ├── mainwindow.cpp / mainwindow.h
│   │   └── ... (dialogs, role classes)
│   └── uploads/                     # Inspection photo storage (ephemeral on Render)
└── docs/
    └── erd/                         # Mermaid UML / ERD diagrams
```

---

## 🚀 Running Locally

```bash
# Clone and install dependencies
git clone https://github.com/Juniusketter/Fire-Watch-App.git
cd Fire-Watch-App
pip install flask flask-cors

# Run the server
python server.py

# Open in browser
http://localhost:5000
```

---

## ⚠️ Known Limitations

- **Photo storage:** Inspection photos are stored ephemerally on Render's free tier and are lost on each redeploy. For production use, integrate cloud storage (S3, Cloudinary, etc.).
- **Render cold start:** The free tier spins down after inactivity. The first request after idle may take 30–60 seconds.
- **SQLite:** Suitable for demo/capstone use. Production deployment should migrate to PostgreSQL.
