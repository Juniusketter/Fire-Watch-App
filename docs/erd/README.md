# Fire‑Watch Database ERD

This folder contains the **Entity–Relationship Diagram** (ERD) for Fire‑Watch, including the Mermaid source used in docs and a high‑resolution rendered image for reports and slide decks.

- **Mermaid source** — editable, reviewable in GitHub
- **Rendered image** — `erd-4k.png` (4K PNG for presentations/print)
- **Scope** — models tenants, third parties, buildings, extinguishers, assignments, reports, photos, and audit trail

> If you’re viewing this on GitHub, Mermaid renders inline below. The same model is also available as a 4K image.

---

## ✅ Mermaid ERD (source)

```mermaid
erDiagram
  COMPANY {
    uuid company_id PK
    string name
    string type  "tenant|third_party"
    datetime created_at
  }

  USER {
    uuid user_id PK
    uuid company_id FK
    string email  "UNIQUE"
    string display_name
    string role   "Admin|Inspector|ThirdPartyAdmin|ThirdPartyInspector"
    string password_hash
    datetime created_at
    datetime updated_at
  }

  ACCESS_GRANT {
    uuid grant_id PK
    uuid tenant_company_id FK
    uuid third_party_company_id FK
    uuid building_id FK  "nullable"
    uuid extinguisher_id FK  "nullable"
    datetime starts_at
    datetime ends_at
    string scope "read|assign|inspect"
  }

  BUILDING {
    uuid building_id PK
    uuid company_id FK
    string address
    string name_or_number
    string description
    datetime created_at
    datetime updated_at
  }

  EXTINGUISHER {
    uuid extinguisher_id PK
    uuid company_id FK
    uuid building_id FK
    string serial_number  "UNIQUE"
    string type           "ABC|CO2|Water|Foam|K-Class"
    string status         "active|outOfService|retired|missing"
    string floor
    string room
    string location_note
    int    inspection_interval_days
    datetime next_due_at
    uuid   last_inspection_report_id FK "nullable"
    datetime created_at
    datetime updated_at
  }

  ASSIGNMENT {
    uuid assignment_id PK
    uuid company_id FK
    uuid building_id FK
    uuid inspector_user_id FK
    datetime created_at
    uuid created_by FK
    datetime due_by
    string status
    string priority
    string idempotent_key
    text   notes
  }

  REPORT {
    uuid report_id PK
    uuid company_id FK
    uuid assignment_id FK
    uuid extinguisher_id FK
    uuid created_by FK
    datetime created_at
    string status
    string submission_mode
    float  gauge_reading_psi
    bool   obstruction
    string media_integrity
    string anomaly_flag
    bool   reset_interval_applied
    text   notes
    string signature
  }

  PHOTO {
    uuid media_id PK
    uuid report_id FK
    string content_type
    int    size_bytes
    string sha256
    int    width
    int    height
    datetime captured_at
  }

  AUDIT_LOG {
    uuid audit_id PK
    uuid company_id FK
    string entity_type
    uuid   entity_id
    uuid   actor_user_id FK
    string action
    string payload_hash
    datetime occurred_at
  }

  COMPANY ||--o{ USER : employs
  COMPANY ||--o{ BUILDING : owns
  COMPANY ||--o{ EXTINGUISHER : owns
  COMPANY ||--o{ ASSIGNMENT : issues
  COMPANY ||--o{ REPORT : records
  COMPANY ||--o{ AUDIT_LOG : produces

  BUILDING ||--o{ EXTINGUISHER : contains
  USER ||--o{ ASSIGNMENT : assigned_to
  USER ||--o{ REPORT : submits
  EXTINGUISHER ||--o{ ASSIGNMENT : targeted_by
  EXTINGUISHER ||--o{ REPORT : inspected_in
  REPORT ||--o{ PHOTO : has

  COMPANY ||--o{ ACCESS_GRANT : grants
  BUILDING ||--o{ ACCESS_GRANT : scoping
  EXTINGUISHER ||--o{ ACCESS_GRANT : scoping
  ACCESS_GRANT }o--|| COMPANY : "for_third_party"
