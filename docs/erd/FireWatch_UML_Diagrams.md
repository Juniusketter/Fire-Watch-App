# FireWatch — UML Diagrams
**Updated:** March 24, 2026  
**Matches:** Current production schema (`server.py` + `FireWatch.db`)

---

## 1. Entity-Relationship Diagram (Database Schema)

```mermaid
erDiagram
  Organizations {
    int org_id PK
    text name
    text address
    text city
    text state
    text zip
    text phone
    text contact_name
    text created_at
    text platform_status "pending|approved|suspended|rejected"
  }

  Users {
    int user_id PK
    text username "UNIQUE"
    text password_hash "SHA-256"
    text role "Admin|Inspector|3rd_Party_Admin|3rd_Party_Inspector"
    text preferences "JSON"
    int org_id FK
  }

  Companies {
    int company_id PK
    int org_id FK
    text name
    text address
    text city
    text state
    text zip
    text phone
    text contact_name
    text created_at
  }

  Buildings {
    int building_id PK
    int company_id FK
    int org_id FK
    text name
    text address
    int floors
    text notes
    text created_at
  }

  Extinguishers {
    int extinguisher_id PK
    text address
    text building_number
    int floor_number
    text room_number
    text location_description
    int inspection_interval_days
    text next_due_date
    int last_report_id FK
    int building_id FK
    int org_id FK
    text ext_type "A|B|C|D|K|ABC|BC"
    real ext_size_lbs
    text serial_number
    text manufacturer
    text model
    text manufacture_date
    text last_annual_date
    text last_6year_date
    text last_hydro_date
    text ext_status "Active|Out of Service|Retired|Missing"
  }

  Assignments {
    int assignment_id PK
    int admin_id FK
    int inspector_id FK
    int extinguisher_id FK
    text status "Pending|In Progress|Complete"
    text notes
    text due_date
    int org_id FK
  }

  Reports {
    int report_id PK
    int extinguisher_id FK
    int inspector_id FK
    text inspection_date
    text photo_path
    text notes
    int org_id FK
  }

  PlatformSettings {
    text key PK
    text value
  }

  PlatformAuditLog {
    int log_id PK
    text timestamp "UTC"
    text actor
    text action
    text target
    text details
  }

  Organizations ||--o{ Users : "has members"
  Organizations ||--o{ Companies : "contains"
  Organizations ||--o{ Buildings : "contains"
  Organizations ||--o{ Extinguishers : "tracks"
  Organizations ||--o{ Assignments : "manages"
  Organizations ||--o{ Reports : "records"

  Companies ||--o{ Buildings : "has"
  Buildings ||--o{ Extinguishers : "contains"

  Users ||--o{ Assignments : "assigned_to (inspector)"
  Users ||--o{ Assignments : "created_by (admin)"
  Users ||--o{ Reports : "submitted_by"

  Extinguishers ||--o{ Assignments : "targeted_by"
  Extinguishers ||--o{ Reports : "inspected_in"
```

---

## 2. C++ Class Diagram (Desktop Application)

```mermaid
classDiagram
  class User {
    -string username
    -string password
    -string category
    +User()
    +User(u, p, c)
    +getUsername() string
    +setUsername(u)
    +getPassword() string
    +setPassword(p)
    +getCategory() string
    +setCategory(c)
  }

  class Admin {
    -string adminID
    -int DBAccess
    +Admin()
    +Admin(u, p, c, id, access)
    +accessDB(access) bool
    +changeDB(access, action, field) bool
    +viewDB(access)
    +generateAssignment(locations, dueDate, invsList, thirdPCompanyName) Assignment
    +getAdminID() string
    +setAdminID(id)
  }

  class Inv {
    -string invID
    +Inv()
    +Inv(u, p, c, id)
    +generateReport(extId, assignId, date, notes) bool
    +getInvID() string
    +setInvID(id)
  }

  class ThirdPAdmin {
    -string thirdPAdminID
    +ThirdPAdmin()
    +ThirdPAdmin(u, p, c, id)
    +getThirdPAdminID() string
    +setThirdPAdminID(id)
  }

  class ThirdPInv {
    -string thirdPInvID
    +ThirdPInv()
    +ThirdPInv(u, p, c, id)
    +generateThirdPReport(extId, assignId, date, notes) bool
    +getThirdPInvID() string
    +setThirdPInvID(id)
  }

  class Assignment {
    -int numExtinguishers
    -int numInvs
    -string id
    -vector~string~ extinguisherLocations
    -string dueDate
    -vector~string~ invsList
    -string thirdPCompanyName
    +Assignment()
    +Assignment(numExt, numInv, id, locs, date, invs)
    +Assignment(numExt, numInv, id, locs, date, company)
  }

  class ExportDialog {
    -QSqlDatabase m_db
    -QTextEdit m_preview
    +ExportDialog(db, parent)
    +onExportPdf()
    +onPrint()
    -loadReportPreview()
  }

  class Dashboard {
    -int m_userId
    -QString m_username
    -QString m_role
    -QSqlDatabase m_db
    +Dashboard(userId, username, role, db, parent)
    +setupTabsForRole()
    +setupExtinguisherToolbar()
    +setupAssignmentToolbar()
    +setupReportToolbar()
    +setupReportExportToolbar()
    +setupUserToolbar()
    +loadExtinguishers()
    +loadAssignments()
    +loadReports()
    +loadUsers()
    +onAddExtinguisher()
    +onEditExtinguisher()
    +onDeleteExtinguisher()
    +onGenerateAssignment()
    +onGenerateReport()
    +onAddUser()
    +onExportReports()
  }

  User <|-- Admin
  User <|-- Inv
  User <|-- ThirdPAdmin
  User <|-- ThirdPInv
  Admin --> Assignment : creates
  Dashboard --> ExportDialog : opens
  Dashboard --> Admin : uses
  Dashboard --> Inv : uses
```

---

## 3. System Architecture Diagram

```mermaid
flowchart TB
  subgraph Client["Client Layer"]
    Browser["Web Browser<br/>(index.html)"]
    QtApp["Qt Desktop App<br/>(dashboard.cpp)"]
    Mobile["Mobile Browser<br/>(Responsive Web)"]
  end

  subgraph Server["Server Layer"]
    Flask["Flask REST API<br/>(server.py)<br/>25+ endpoints"]
    Static["Static File Server<br/>(/uploads, index.html)"]
  end

  subgraph Data["Data Layer"]
    SQLite["SQLite Database<br/>(FireWatch.db)<br/>9 tables"]
    Uploads["File Storage<br/>(src/uploads/)"]
  end

  subgraph Platform["Platform Admin"]
    PlatformUI["Platform Dashboard<br/>(⚙ Platform tab)"]
    AuditLog["Audit Log<br/>(PlatformAuditLog)"]
    Settings["Platform Settings<br/>(PlatformSettings)"]
  end

  Browser -->|"fetch() API calls"| Flask
  Mobile -->|"fetch() API calls"| Flask
  QtApp -->|"QSqlQuery (direct)"| SQLite
  Flask -->|"sqlite3 read/write"| SQLite
  Flask -->|"serve/store photos"| Uploads
  Static -->|"serves"| Browser
  PlatformUI -->|"/api/platform/*"| Flask
  Flask -->|"logs actions"| AuditLog
  Flask -->|"reads config"| Settings

  style Client fill:#1a1e26,stroke:#e85d04,color:#e8eaf0
  style Server fill:#1a1e26,stroke:#0d9488,color:#e8eaf0
  style Data fill:#1a1e26,stroke:#1d4ed8,color:#e8eaf0
  style Platform fill:#1a1e26,stroke:#7c3aed,color:#e8eaf0
```

---

## 4. User Flow / Activity Diagram

```mermaid
flowchart TD
  Start([User Opens FireWatch]) --> AuthChoice{Login or Register?}

  AuthChoice -->|Sign In| Login[Enter Username + Password]
  AuthChoice -->|Register| Register[Create Org + Admin Account]
  AuthChoice -->|Platform| PlatformLogin[Enter Platform Password]

  Login --> CheckStatus{Org Status?}
  Register --> Pending[Org Pending Approval]
  PlatformLogin --> PlatformDash[Platform Admin Dashboard]

  CheckStatus -->|approved| Dashboard[Main Dashboard]
  CheckStatus -->|pending| PendingScreen[Pending Overlay]
  CheckStatus -->|suspended| SuspendedScreen[Suspended Overlay]

  PlatformDash --> ManageOrgs[View/Approve/Suspend/Delete Orgs]
  PlatformDash --> ViewAudit[Activity Audit Log]
  PlatformDash --> DrillOrg[Drill Into Org Data]
  PlatformDash --> ToggleApproval[Toggle Approval Requirement]

  Dashboard --> RoleCheck{User Role?}

  RoleCheck -->|Admin| AdminView[Extinguishers + Assignments + Reports + Users]
  RoleCheck -->|Inspector| InvView[My Assignments + My Reports]
  RoleCheck -->|3rd Party Admin| TPAView[Assignments + My Team]
  RoleCheck -->|3rd Party Inspector| TPIView[My Assignments]

  AdminView --> DrillDown[Companies → Buildings → Extinguishers]
  AdminView --> ManageUsers[Add / Edit / Delete Users]
  AdminView --> GenAssign[Generate Assignment]
  AdminView --> ViewReports[View Reports + Export PDF/CSV/Word]
  AdminView --> Search[Global Search Bar]
  AdminView --> NFPA[NFPA 10 Metadata on Extinguishers]

  InvView --> SubmitReport[Submit Report + Photo Upload]
  InvView --> ViewAssign[View My Assignments]

  GenAssign --> Inspector[Inspector Receives Assignment]
  Inspector --> SubmitReport
  SubmitReport --> ReportStored[Report Saved to Database]

  style Start fill:#e85d04,stroke:#e85d04,color:#fff
  style Dashboard fill:#0d9488,stroke:#0d9488,color:#fff
  style PlatformDash fill:#7c3aed,stroke:#7c3aed,color:#fff
  style PendingScreen fill:#e85d04,stroke:#e85d04,color:#fff
```

---

## 5. API Endpoint Map

```mermaid
flowchart LR
  subgraph Auth["Authentication"]
    POST_login["POST /api/login"]
    POST_register["POST /api/register"]
    POST_platform["POST /api/platform/login"]
  end

  subgraph CRUD["Core CRUD"]
    Companies_API["GET|POST|PUT|DELETE<br/>/api/companies"]
    Buildings_API["GET|POST|PUT|DELETE<br/>/api/buildings"]
    Ext_API["GET|POST|PUT|DELETE<br/>/api/extinguishers"]
    Assign_API["GET|POST|PUT<br/>/api/assignments"]
    Reports_API["GET|POST<br/>/api/reports"]
    Users_API["GET|POST|PUT|DELETE<br/>/api/users"]
  end

  subgraph Features["Features"]
    Upload["POST /api/upload"]
    Search["GET /api/search"]
    Stats["GET /api/stats"]
    Notifs["GET /api/notifications"]
    Prefs["GET|PUT /api/users/:id/preferences"]
    Password["PUT /api/users/:id/password"]
  end

  subgraph PlatformAPI["Platform Admin"]
    POrgs["GET /api/platform/organizations"]
    POrgDetail["GET /api/platform/organizations/:id/details"]
    POrgStatus["PUT /api/platform/organizations/:id/status"]
    POrgDelete["DELETE /api/platform/organizations/:id"]
    PSettings["GET|PUT /api/platform/settings"]
    PAudit["GET /api/platform/audit"]
  end

  style Auth fill:#e85d04,stroke:#e85d04,color:#fff
  style CRUD fill:#0d9488,stroke:#0d9488,color:#fff
  style Features fill:#1d4ed8,stroke:#1d4ed8,color:#fff
  style PlatformAPI fill:#7c3aed,stroke:#7c3aed,color:#fff
```
