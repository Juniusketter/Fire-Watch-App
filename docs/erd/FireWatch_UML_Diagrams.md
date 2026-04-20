# FireWatch — UML Diagrams
**Updated:** April 20, 2026
**Matches:** Sprint 5 final production state (`server.py` + `FireWatch.db` + Qt desktop app)

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
    text password_hash "SHA-256 client-side"
    text role "Admin|Inspector|3rd_Party_Admin|3rd_Party_Inspector|Client"
    text first_name
    text last_name
    text email
    text phone
    text cert_number "Inspector certification number"
    text subcontractor_company_name "3rd-party affiliation"
    text preferences "JSON blob"
    int org_id FK
    text account_status "pending|approved|suspended|denied"
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
    text last_inspection_date "routine monthly"
    int last_report_id FK
    int building_id FK
    int org_id FK
    text ext_type "A|B|C|D|K|ABC|BC"
    real ext_size_lbs
    text serial_number
    text manufacturer
    text model
    text manufacture_date
    text last_annual_date "NFPA annual service"
    text last_6year_date "NFPA 6-year maintenance"
    text last_hydro_date "NFPA hydrostatic test"
    text dot_cert_no "DOT certification number"
    text ext_status "Active|Out of Service|Retired|Missing"
  }

  Assignments {
    int assignment_id PK
    int admin_id FK
    int inspector_id FK
    int extinguisher_id FK
    text status "Pending Inspection|In Progress|Inspection Complete"
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
    text service_type "Routine Inspection|Annual|6-Year|Hydrostatic|Replacement"
    text inspection_result "Pass|Fail|Needs Repair|Conditional"
    int chk_mounted "NFPA 7.2.2 - properly mounted"
    int chk_access "NFPA 7.2.2 - unobstructed access"
    int chk_pressure "NFPA 7.2.2 - pressure in range"
    int chk_seal "NFPA 7.2.2 - tamper seal intact"
    int chk_damage "NFPA 7.2.2 - no visible damage"
    int chk_nozzle "NFPA 7.2.2 - nozzle/hose clear"
    int chk_label "NFPA 7.2.2 - label legible"
    int chk_weight "NFPA 7.2.2 - weight acceptable"
    text is_voided "0|1"
    text void_reason
  }

  Messages {
    int message_id PK
    int org_id FK
    int sender_id FK
    int recipient_id FK "null = broadcast"
    text subject
    text body
    int is_read "0|1"
    int is_broadcast "0|1"
    text broadcast_role "role filter or empty"
    text created_at
  }

  PlatformAnnouncements {
    int announcement_id PK
    text title
    text body
    text priority "info|warning|critical"
    text created_at
    text expires_at "nullable"
    int is_active "0|1"
  }

  ServiceRequests {
    int request_id PK
    int org_id FK
    int company_id FK
    int extinguisher_id FK
    text requested_by "username"
    text request_type "Service/Repair|Recharge|Parts|Other"
    text part_number "optional"
    int quantity
    text vendor "optional"
    real estimated_cost
    text notes
    text status "Pending|In Progress|Fulfilled|Rejected"
    text admin_notes
    text created_at
    text updated_at
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
  Extinguishers }o--|| Reports : "last_report_id"
  Extinguishers ||--o{ ServiceRequests : "requested_for"

  Organizations ||--o{ Messages : "scoped_to"
  Users ||--o{ Messages : "sends"
  Users ||--o{ Messages : "receives"

  Organizations ||--o{ ServiceRequests : "scoped_to"
  Companies ||--o{ ServiceRequests : "requested_by_client"
```

---

## 2. C++ Class Diagram (Desktop Application)

```mermaid
classDiagram
  %% ── Legacy role stub classes (Sprint 1–2 scaffolding, not used by Dashboard) ──
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
    +accessDB(access) bool
    +changeDB(access, action, field) bool
    +generateAssignment(locs, dueDate, invsList, company) Assignment
    +getAdminID() string
  }

  class Inv {
    -string invID
    +Inv()
    +generateReport(extId, assignId, date, notes) bool
    +getInvID() string
  }

  class ThirdPAdmin {
    -string thirdPAdminID
    +ThirdPAdmin()
    +getThirdPAdminID() string
  }

  class ThirdPInv {
    -string thirdPInvID
    +ThirdPInv()
    +generateThirdPReport(extId, assignId, date, notes) bool
    +getThirdPInvID() string
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
  }

  %% ── Active Qt Widgets application classes ──
  class MainWindow {
    -Ui_MainWindow ui
    -int m_userId
    -QString m_username
    -QString m_role
    +MainWindow(parent)
    +~MainWindow()
    -onLoginClicked()
    -openDatabase() bool
  }

  class Dashboard {
    -int m_userId
    -QString m_username
    -QString m_role
    -int m_orgId
    -QSqlDatabase m_db
    +Dashboard(userId, username, role, orgId, db, parent)
    +~Dashboard()
    -isAdmin() bool
    -isInv() bool
    -isThirdPAdmin() bool
    -isThirdPInv() bool
    -isClient() bool
    -isPlatformAdmin() bool
    -setupTabsForRole()
    -setupExtinguisherToolbar()
    -setupAssignmentToolbar()
    -setupReportToolbar()
    -setupReportExportToolbar()
    -setupUserToolbar()
    -loadExtinguishers()
    -loadAssignments()
    -loadReports()
    -loadUsers()
    -updateStats()
    -onLogoutClicked()
    -onRefreshClicked()
    -onAddExtinguisher()
    -onEditExtinguisher()
    -onDeleteExtinguisher()
    -onGenerateAssignment()
    -onGenerateReport()
    -onAddUser()
    -onEditUser()
    -onDeleteUser()
    -onExportReports()
    -checkDueDateAlerts()
    -selectedExtinguisherId() int
    -selectedUserId() int
    -fillTable(table, query)
    -showEmptyMessage(table, message)
  }

  class ExtDialog {
    -QString m_mode
    -QLineEdit m_address
    -QLineEdit m_buildingNumber
    -QSpinBox m_floorNumber
    -QLineEdit m_roomNumber
    -QLineEdit m_locationDescription
    -QSpinBox m_inspectionInterval
    -QDateEdit m_nextDueDate
    -QComboBox m_extType
    -QDoubleSpinBox m_extSize
    -QLineEdit m_serial
    -QLineEdit m_manufacturer
    -QLineEdit m_model
    -QDateEdit m_manufactureDate
    -QDateEdit m_lastAnnualDate
    -QDateEdit m_last6YearDate
    -QDateEdit m_lastHydroDate
    -QComboBox m_extStatus
    +ExtDialog(mode, parent)
    +setAddress(v) / address() QString
    +setExtType(v) / extType() QString
    +setSerialNumber(v) / serialNumber() QString
    +setExtStatus(v) / extStatus() QString
  }

  class AssignDialog {
    -QComboBox m_extinguisher
    -QComboBox m_inspector
    -QDateEdit m_dueDate
    -QLineEdit m_notes
    +AssignDialog(parent)
    +setExtinguishers(items)
    +setInspectors(items)
    +extinguisherId() int
    +inspectorId() int
    +dueDate() QString
    +notes() QString
  }

  class ReportDialog {
    -QComboBox m_assignment
    -QComboBox m_extinguisher
    -QComboBox m_serviceType
    -QComboBox m_result
    -QDateEdit m_inspectionDate
    -QTextEdit m_notes
    +ReportDialog(parent)
    +setAssignments(items)
    +setExtinguishers(items)
    +assignmentId() int
    +extinguisherId() int
    +serviceType() QString
    +result() QString
    +inspectionDate() QString
    +notes() QString
  }

  class AddUserDialog {
    -QLineEdit m_username
    -QLineEdit m_password
    -QLineEdit m_confirm
    -QComboBox m_role
    -QLabel m_errorLabel
    +AddUserDialog(parent)
    +username() QString
    +password() QString
    +role() QString
  }

  class EditUserDialog {
    -QLabel m_usernameDisplay
    -QComboBox m_role
    -QLineEdit m_firstName
    -QLineEdit m_lastName
    -QLineEdit m_email
    -QLineEdit m_phone
    -QLineEdit m_certNumber
    -QLineEdit m_subcontractorCompany
    -QCheckBox m_resetPassword
    -QLineEdit m_newPassword
    -QLineEdit m_confirmPassword
    -QLabel m_errorLabel
    +EditUserDialog(parent)
    +setDisplayUsername(v)
    +setRole(v) / role() QString
    +setFirstName(v) / firstName() QString
    +setLastName(v) / lastName() QString
    +setEmail(v) / email() QString
    +setPhone(v) / phone() QString
    +setCertNumber(v) / certNumber() QString
    +setSubcontractorCompany(v) / subcontractorCompany() QString
    +resetPassword() bool
    +newPassword() QString
  }

  class DueDateSummaryDialog {
    -QList~DueDateItem~ items
    -int overdueCount
    -int upcomingCount
    +DueDateSummaryDialog(items, parent)
    -buildTable(items)
  }

  class ExportDialog {
    -QSqlDatabase m_db
    -QTextEdit m_preview
    +ExportDialog(db, parent)
    +onExportPdf()
    +onPrint()
    -loadReportPreview()
  }

  %% Legacy hierarchy (Sprint 1 stubs — not linked to Dashboard)
  User <|-- Admin
  User <|-- Inv
  User <|-- ThirdPAdmin
  User <|-- ThirdPInv
  Admin --> Assignment : creates

  %% Active application flow
  MainWindow --> Dashboard : opens on login
  Dashboard --> ExtDialog : add/edit extinguisher
  Dashboard --> AssignDialog : generate assignment
  Dashboard --> ReportDialog : submit report
  Dashboard --> AddUserDialog : add user
  Dashboard --> EditUserDialog : edit user
  Dashboard --> DueDateSummaryDialog : shows on login
  Dashboard --> ExportDialog : export/print reports
```

---

## 3. System Architecture Diagram

```mermaid
flowchart TB
  subgraph Client["Client Layer"]
    Landing["Landing Page\nlanding.html served at /"]
    Browser["Web App\nindex.html served at /app"]
    QtApp["Qt Desktop App\ndashboard.cpp + mainwindow.cpp"]
    Mobile["Mobile Browser\nResponsive Web"]
    AuditorView["Auditor View\nRead-only token URL"]
  end

  subgraph Server["Server Layer"]
    Flask["Flask REST API\nserver.py\n60+ endpoints"]
    Static["Static File Server\n/ and /app and /uploads"]
  end

  subgraph Data["Data Layer"]
    SQLite["SQLite Database\nFireWatch.db\n12 tables"]
    Uploads["File Storage\nsrc/uploads/"]
  end

  subgraph Platform["Platform Admin"]
    PlatformUI["Platform Dashboard\nembedded in index.html"]
    AuditLog["Audit Log\nPlatformAuditLog"]
    Settings["Platform Settings\nPlatformSettings"]
    Compliance["Compliance Report\n/api/platform/compliance"]
    Announcements["Announcements\n/api/platform/announcements"]
  end

  Landing -->|user navigates to /app| Browser
  Browser -->|fetch REST calls| Flask
  Mobile -->|fetch REST calls| Flask
  QtApp -->|QSqlQuery direct to DB| SQLite
  Flask -->|sqlite3 read/write| SQLite
  Flask -->|serve and store photos| Uploads
  Static -->|serves| Browser
  Static -->|serves| Landing
  AuditorView -->|GET /api/auditor/:token| Flask
  PlatformUI -->|/api/platform/ routes| Flask
  Flask -->|writes| AuditLog
  Flask -->|reads config| Settings
  Flask -->|cross-org reporting| Compliance
  Flask -->|broadcasts| Announcements

  style Client fill:#1a1e26,stroke:#e85d04,color:#e8eaf0
  style Server fill:#1a1e26,stroke:#0d9488,color:#e8eaf0
  style Data fill:#1a1e26,stroke:#1d4ed8,color:#e8eaf0
  style Platform fill:#1a1e26,stroke:#7c3aed,color:#e8eaf0
```

---

## 4. User Flow / Activity Diagram

```mermaid
flowchart TD
  Start([User Opens FireWatch]) --> Landing[Landing Page]
  Landing --> AuthChoice{Sign In / Register / Platform?}

  AuthChoice -->|Sign In| Login[Enter Username + Password]
  AuthChoice -->|Register Org| Register[Create Org + First Admin Account]
  AuthChoice -->|Platform| PlatformLogin[Enter Platform Password]
  AuthChoice -->|User Self-Register| Signup[Sign Up under existing Org]

  Login --> CheckOrgStatus{Org Status?}
  Register --> OrgPending[Org Pending Platform Approval]
  Signup --> UserPending[User Pending Admin Approval]
  PlatformLogin --> PlatformDash[Platform Admin Dashboard]

  CheckOrgStatus -->|approved| CheckUserStatus{User account_status?}
  CheckOrgStatus -->|pending| OrgPendingOverlay[Org Pending Overlay]
  CheckOrgStatus -->|suspended| SuspendedOverlay[Org Suspended Overlay]

  CheckUserStatus -->|approved| Dashboard[Main Dashboard]
  CheckUserStatus -->|pending| UserPendingOverlay[Awaiting Admin Approval]
  CheckUserStatus -->|suspended or denied| AccessDenied[Access Denied Screen]

  PlatformDash --> ManageOrgs[View / Approve / Suspend / Delete Orgs]
  PlatformDash --> ViewAudit[Activity Audit Log]
  PlatformDash --> DrillOrg[Drill Into Org Data — Read-Only]
  PlatformDash --> ComplianceReport[Platform Compliance Report]
  PlatformDash --> ManageAnnouncements[Create / Edit / Expire Announcements]

  Dashboard --> RoleCheck{User Role?}

  RoleCheck -->|Admin| AdminView[Extinguishers + Assignments + Reports + Users]
  RoleCheck -->|Inspector| InvView[My Assignments + Submit Reports]
  RoleCheck -->|3rd_Party_Admin| TPAView[Team Assignments + My Inspectors]
  RoleCheck -->|3rd_Party_Inspector| TPIView[My Assigned Extinguisher]
  RoleCheck -->|Client| ClientView[My Extinguishers — Read-Only + QR + NFPA Form + Service Requests]

  AdminView --> DueDateAlert[Due Date Summary Alert on Login]
  AdminView --> DrillDown[Companies to Buildings to Extinguishers]
  AdminView --> ManageUsers[Add / Edit / Approve / Suspend / Delete Users]
  AdminView --> GenAssign[Generate Assignment with Notes]
  AdminView --> ViewReports[View Reports + Approve/Void + Export PDF/CSV/Word]
  AdminView --> PriorityQueue[Priority Queue — Inspection Urgency Scoring]
  AdminView --> Search[Global Search Bar]
  AdminView --> AuditorTokens[Generate Auditor Read-Only Token URL]
  AdminView --> Messaging[In-App Messaging + Broadcast]
  AdminView --> Calendar[Monthly Inspection Calendar]

  GenAssign --> Inspector[Inspector Receives Assignment]
  Inspector --> SubmitReport[Submit Report + Service Type + Result + NFPA Checklist + Photo]
  InvView --> SubmitReport
  SubmitReport --> ReportStored[Report Saved — Immutable NFPA Audit Trail]

  ClientView --> ServiceRequest[Submit Service Request for Deficiency]
  ServiceRequest --> AdminReview[Admin Updates Status + Notes]

  AuditorURL([Auditor Token URL]) --> ReadOnlyDash[Read-Only Org Dashboard]

  style Start fill:#e85d04,stroke:#e85d04,color:#fff
  style Dashboard fill:#0d9488,stroke:#0d9488,color:#fff
  style PlatformDash fill:#7c3aed,stroke:#7c3aed,color:#fff
  style OrgPendingOverlay fill:#b45309,stroke:#b45309,color:#fff
  style AuditorURL fill:#1d4ed8,stroke:#1d4ed8,color:#fff
```

---

## 5. API Endpoint Map

```mermaid
flowchart LR
  subgraph Static["Static Routes"]
    GET_root["GET /\nlanding.html"]
    GET_app["GET /app\nindex.html"]
    GET_static["GET /:filename\nUI assets"]
    GET_uploads["GET /uploads/:filename\nInspection photos"]
  end

  subgraph Auth["Authentication"]
    POST_register["POST /api/register\nOrg + first Admin signup"]
    POST_signup["POST /api/signup\nUser self-registration"]
    POST_login["POST /api/login"]
    POST_plat_login["POST /api/platform/login"]
  end

  subgraph Users["Users"]
    GET_users["GET /api/users"]
    POST_users["POST /api/users"]
    PUT_user["PUT /api/users/:id"]
    DEL_user["DELETE /api/users/:id"]
    PUT_pw["PUT /api/users/:id/password"]
    GET_prefs["GET /api/users/:id/preferences"]
    PUT_prefs["PUT /api/users/:id/preferences"]
    POST_status["POST /api/users/:id/status\napprove|suspend|deny"]
    GET_pending["GET /api/users/pending"]
  end

  subgraph CRUD["Core CRUD"]
    Cos["GET|POST|PUT|DELETE\n/api/companies"]
    Bldgs["GET|POST|PUT|DELETE\n/api/buildings"]
    Exts["GET|POST\n/api/extinguishers"]
    ExtID["GET|PUT|DELETE\n/api/extinguishers/:id"]
    Assigns["GET|POST|PUT\n/api/assignments"]
    Rpts["GET|POST\n/api/reports"]
  end

  subgraph Actions["Workflow Actions"]
    POST_approve["POST /api/reports/:id/approve"]
    POST_void["POST /api/reports/:id/void"]
    POST_condemn["POST /api/extinguishers/:id/condemn"]
    POST_bulk_import["POST /api/extinguishers/bulk"]
    POST_bulk_assign["POST /api/assignments/bulk"]
  end

  subgraph Features["Features"]
    POST_upload["POST /api/upload\nInspection photo"]
    GET_search["GET /api/search"]
    GET_stats["GET /api/stats"]
    GET_notifs["GET /api/notifications"]
    GET_ann["GET /api/announcements"]
    GET_msgs["GET|POST /api/messages"]
    GET_priority["GET /api/priority-queue"]
    GET_qr["GET /api/extinguishers/:id/qr"]
    GET_nfpa["GET /api/extinguishers/:id/nfpa-form"]
    GET_calendar["GET /api/assignments/calendar"]
  end

  subgraph Auditor["Auditor — Public Read-Only"]
    POST_tok["POST /api/auditor/tokens"]
    GET_tok["GET /api/auditor/:token"]
    GET_toks["GET /api/auditor/tokens"]
    DEL_tok["DELETE /api/auditor/tokens/:id"]
  end

  subgraph ServiceReq["Service Requests"]
    GET_sr["GET /api/service-requests"]
    POST_sr["POST /api/service-requests"]
    PUT_sr["PUT /api/service-requests/:id"]
  end

  subgraph ClientAPI["Client Role"]
    GET_client["GET /api/client/stats"]
    GET_deficiencies["GET /api/client/deficiencies"]
  end

  subgraph PlatformAPI["Platform Admin"]
    GET_orgs["GET /api/platform/organizations"]
    PUT_org_status["PUT /api/platform/organizations/:id/status"]
    DEL_org["DELETE /api/platform/organizations/:id"]
    GET_org_detail["GET /api/platform/organizations/:id/details"]
    GPSET["GET|PUT /api/platform/settings"]
    GET_audit["GET /api/platform/audit"]
    GET_comp["GET /api/platform/compliance"]
    CRUD_ann["GET|POST|PUT|DELETE\n/api/platform/announcements"]
  end

  style Static fill:#374151,stroke:#6b7280,color:#e8eaf0
  style Auth fill:#e85d04,stroke:#e85d04,color:#fff
  style Users fill:#0d9488,stroke:#0d9488,color:#fff
  style CRUD fill:#0d9488,stroke:#0d9488,color:#fff
  style Actions fill:#b45309,stroke:#b45309,color:#fff
  style Features fill:#1d4ed8,stroke:#1d4ed8,color:#fff
  style Auditor fill:#1d4ed8,stroke:#1d4ed8,color:#fff
  style ServiceReq fill:#065f46,stroke:#065f46,color:#fff
  style ClientAPI fill:#065f46,stroke:#065f46,color:#fff
  style PlatformAPI fill:#7c3aed,stroke:#7c3aed,color:#fff
```

---

## 6. Role Permission Matrix

```mermaid
flowchart LR
  subgraph Roles["6 Roles"]
    RAdmin["Admin\nfull org access"]
    RInsp["Inspector\nbuilding-scoped"]
    RTPA["3rd_Party_Admin\nteam-scoped"]
    RTPI["3rd_Party_Inspector\nunit-scoped"]
    RClient["Client\nread-only + service requests"]
    RPlatform["Platform Admin\ncross-org SaaS owner"]
  end

  subgraph Perms["Feature Access"]
    PExt["Extinguishers\nfull CRUD + condemn + bulk import"]
    PAssign["Assignments\ncreate + bulk + calendar view"]
    PReports["Reports\nsubmit + approve + void + export"]
    PUsers["Users\nadd + approve + suspend + delete"]
    PCompanies["Companies + Buildings\nfull CRUD drill-down"]
    PQR["QR Code + NFPA Form\ngenerate + print"]
    PSearch["Global Search"]
    PAudit["Auditor Tokens\ngenerate read-only URLs"]
    PMessages["Messaging\ndirect + group + broadcast"]
    PPriority["Priority Queue\nurgency scoring"]
    PClient["My Extinguishers\nread-only + service requests"]
    PPlatform["All Orgs + Audit\nAnnouncements + Compliance"]
  end

  RAdmin --> PExt
  RAdmin --> PAssign
  RAdmin --> PReports
  RAdmin --> PUsers
  RAdmin --> PCompanies
  RAdmin --> PQR
  RAdmin --> PSearch
  RAdmin --> PAudit
  RAdmin --> PMessages
  RAdmin --> PPriority

  RInsp --> PReports
  RInsp --> PAssign

  RTPA --> PAssign
  RTPA --> PReports

  RTPI --> PReports

  RClient --> PClient
  RClient --> PQR

  RPlatform --> PPlatform

  style Roles fill:#1a1e26,stroke:#e85d04,color:#e8eaf0
  style Perms fill:#1a1e26,stroke:#0d9488,color:#e8eaf0
```
