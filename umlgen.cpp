// SPDX-License-Identifier: MIT
// Fire Watch App - UML Generator (C++17)
// Writes Sprint 2 Mermaid source files (.md and .mmd) under ./docs

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct FileDef { std::string path; std::string content; };

static const char* ACTIVITY_MD = R"MD(# Sprint 2 – Activity Diagram

```mermaid
flowchart TD
  A[Start Sprint 2] --> B[Implement Web UI\n(React + tables/filters)]
  B --> C[Refactor Components for RN parity\n(shared UI patterns)]
  C --> D[Implement iOS-first Mobile UI\n(React Native)]
  D --> E[Identify & Wire Field Flags\n(required, readOnly, inputType, min/max...)]
  E --> F[Implement Assignment Generation & Sending\n(Admin -> Inspector)]
  F --> G[Implement Report Creation & Sending\n(Inspector -> Admin)]
  G --> H[Define Static Values & Admin Controls\n(intervals, enums, templates)]
  H --> I[DB <-> FE/BE Integration Tests]
  I --> J[Component Communication Tests\n(P2P/relay path, idempotency)]
  J --> K{Meets Acceptance Criteria?}
  K -- No --> B
  K -- Yes --> L[Demo & Close Sprint 2]
```
)MD";

static const char* ACTIVITY_MMD = R"MMD(flowchart TD
  A[Start Sprint 2] --> B[Implement Web UI\n(React + tables/filters)]
  B --> C[Refactor Components for RN parity\n(shared UI patterns)]
  C --> D[Implement iOS-first Mobile UI\n(React Native)]
  D --> E[Identify & Wire Field Flags\n(required, readOnly, inputType, min/max...)]
  E --> F[Implement Assignment Generation & Sending\n(Admin -> Inspector)]
  F --> G[Implement Report Creation & Sending\n(Inspector -> Admin)]
  G --> H[Define Static Values & Admin Controls\n(intervals, enums, templates)]
  H --> I[DB <-> FE/BE Integration Tests]
  I --> J[Component Communication Tests\n(P2P/relay path, idempotency)]
  J --> K{Meets Acceptance Criteria?}
  K -- No --> B
  K -- Yes --> L[Demo & Close Sprint 2]
)MMD";

static const char* USECASE_MD = R"MD(# Sprint 2 – Use Case Diagram

```mermaid
flowchart LR
  subgraph Actors
    Admin([Administrator])
    TPA([Third-Party Admin])
    Insp([Inspector])
    TPI([Third-Party Inspector])
  end

  subgraph System[Fire Watch App]
    U1((Filter Extinguishers Due\n<=30 days))
    U2((Generate & Send Assignments))
    U3((View Daily Assignments))
    U4((Capture Report\nphotos + notes))
    U5((Send Report))
    U6((Manage Static Values\nintervals/enums/templates))
    U7((Search reports/assignments/extinguishers))
  end

  Admin --> U1
  Admin --> U2
  TPA --> U2
  Insp --> U3
  TPI --> U3
  Insp --> U4
  Insp --> U5
  TPI --> U4
  TPI --> U5
  Admin --> U6
  TPA --> U6
  Admin --> U7
  TPA --> U7
  Insp --> U7
  TPI --> U7
```
)MD";

static const char* USECASE_MMD = R"MMD(flowchart LR
  subgraph Actors
    Admin([Administrator])
    TPA([Third-Party Admin])
    Insp([Inspector])
    TPI([Third-Party Inspector])
  end

  subgraph System[Fire Watch App]
    U1((Filter Extinguishers Due\n<=30 days))
    U2((Generate & Send Assignments))
    U3((View Daily Assignments))
    U4((Capture Report\nphotos + notes))
    U5((Send Report))
    U6((Manage Static Values\nintervals/enums/templates))
    U7((Search reports/assignments/extinguishers))
  end

  Admin --> U1
  Admin --> U2
  TPA --> U2
  Insp --> U3
  TPI --> U3
  Insp --> U4
  Insp --> U5
  TPI --> U4
  TPI --> U5
  Admin --> U6
  TPA --> U6
  Admin --> U7
  TPA --> U7
  Insp --> U7
  TPI --> U7
)MMD";

static const char* COMPONENTS_MD = R"MD(# Sprint 2 – Component Diagram

```mermaid
flowchart TB
  subgraph UI[UI Subsystem]
    WebUI[Web UI (React)]
    MobileUI[Mobile UI (React Native iOS-first)]
  end

  subgraph FE[Front-End Services]
    FieldFlagGuard[FieldFlagGuard\n(flags -> inputs, validation)]
    LocalStore[Local Storage\n(SQLite/IndexedDB)]
    SyncQueue[Local Sync Queue\n(idempotency, retries)]
  end

  subgraph SUBMIT[Submission Microservice]
    Gate[API Gateway\nRBAC, scopes]
    Queue[Submission Queue\n(store-and-forward)]
  end

  subgraph AdminDB[Extinguisher Database Subsystem]
    DB[(Tenant-Scoped DB)]
    Audit[(Immutable Audit Log)]
  end

  subgraph P2P[P2P/Relay Layer]
    WebRTC[WebRTC + TURN Relay]
  end

  WebUI --> FieldFlagGuard
  MobileUI --> FieldFlagGuard
  FieldFlagGuard --> LocalStore
  FieldFlagGuard --> SyncQueue
  SyncQueue <--> WebRTC
  SyncQueue --> Gate
  Gate --> Queue
  Queue --> DB
  DB --> Audit
```
)MD";

static const char* COMPONENTS_MMD = R"MMD(flowchart TB
  subgraph UI[UI Subsystem]
    WebUI[Web UI (React)]
    MobileUI[Mobile UI (React Native iOS-first)]
  end

  subgraph FE[Front-End Services]
    FieldFlagGuard[FieldFlagGuard\n(flags -> inputs, validation)]
    LocalStore[Local Storage\n(SQLite/IndexedDB)]
    SyncQueue[Local Sync Queue\n(idempotency, retries)]
  end

  subgraph SUBMIT[Submission Microservice]
    Gate[API Gateway\nRBAC, scopes]
    Queue[Submission Queue\n(store-and-forward)]
  end

  subgraph AdminDB[Extinguisher Database Subsystem]
    DB[(Tenant-Scoped DB)]
    Audit[(Immutable Audit Log)]
  end

  subgraph P2P[P2P/Relay Layer]
    WebRTC[WebRTC + TURN Relay]
  end

  WebUI --> FieldFlagGuard
  MobileUI --> FieldFlagGuard
  FieldFlagGuard --> LocalStore
  FieldFlagGuard --> SyncQueue
  SyncQueue <--> WebRTC
  SyncQueue --> Gate
  Gate --> Queue
  Queue --> DB
  DB --> Audit
)MMD";

static const char* SEQ_ASSIGN_MD = R"MD(# Sprint 2 – Sequence Diagram (Assignment Delivery)

```mermaid
sequenceDiagram
  autonumber
  participant AdminUI as Admin UI (Web)
  participant Flag as FieldFlagGuard
  participant FE as Local Store/Sync Queue
  participant P2P as P2P/Relay
  participant InspApp as Inspector App (Mobile)
  participant Gate as API Gateway
  participant Queue as Submission Queue
  participant DB as Admin DB

  AdminUI->>Flag: Fill form (dueBy, target, notes) with flags validation
  Flag-->>AdminUI: Validated payload (idempotentKey set)
  AdminUI->>FE: Save Assignment (status=assigned)
  FE->>P2P: Deliver assignment (messageId, idempotencyKey)
  alt P2P available
    P2P-->>InspApp: Assignment message
  else Relay fallback
    FE->>Gate: POST /assignments (scope: admin)
    Gate->>Queue: enqueue
    Queue-->>InspApp: push/relay delivery
  end
  InspApp->>FE: Store assignment locally; show in Daily Assignments
  InspApp-->>AdminUI: (Ack via relay/P2P as needed)
  AdminUI->>DB: (Optional) persist admin-side copy & audit
  DB-->>AdminUI: OK + audit entry
```
)MD";

static const char* SEQ_ASSIGN_MMD = R"MMD(sequenceDiagram
  autonumber
  participant AdminUI as Admin UI (Web)
  participant Flag as FieldFlagGuard
  participant FE as Local Store/Sync Queue
  participant P2P as P2P/Relay
  participant InspApp as Inspector App (Mobile)
  participant Gate as API Gateway
  participant Queue as Submission Queue
  participant DB as Admin DB

  AdminUI->>Flag: Fill form (dueBy, target, notes) with flags validation
  Flag-->>AdminUI: Validated payload (idempotentKey set)
  AdminUI->>FE: Save Assignment (status=assigned)
  FE->>P2P: Deliver assignment (messageId, idempotencyKey)
  alt P2P available
    P2P-->>InspApp: Assignment message
  else Relay fallback
    FE->>Gate: POST /assignments (scope: admin)
    Gate->>Queue: enqueue
    Queue-->>InspApp: push/relay delivery
  end
  InspApp->>FE: Store assignment locally; show in Daily Assignments
  InspApp-->>AdminUI: (Ack via relay/P2P as needed)
  AdminUI->>DB: (Optional) persist admin-side copy & audit
  DB-->>AdminUI: OK + audit entry
)MMD";

static const char* SEQ_REPORT_MD = R"MD(# Sprint 2 – Sequence Diagram (Report Submission)

```mermaid
sequenceDiagram
  autonumber
  participant InspApp as Inspector App (Mobile)
  participant Flag as FieldFlagGuard
  participant FE as Local Store/Sync Queue
  participant P2P as P2P/Relay
  participant Gate as API Gateway
  participant Queue as Submission Queue
  participant AdminUI as Admin UI (Web)
  participant DB as Admin DB
  participant Audit as Audit Log

  InspApp->>Flag: Capture notes + photos (media.hash, size limits)
  Flag-->>InspApp: Validated report (status=submitted)
  InspApp->>FE: Save Report (offline allowed)
  FE->>P2P: Try P2P delivery (idempotencyKey, contentHash)
  alt P2P success
    P2P-->>AdminUI: Report received
  else fallback relay
    FE->>Gate: POST /submit-report (scope: submit-report)
    Gate->>Queue: enqueue
    Queue-->>AdminUI: notify + payload
  end
  AdminUI->>DB: Upsert latest report & reset interval
  DB-->>AdminUI: OK
  AdminUI->>Audit: Append immutable entry (who/when/hash)
  Audit-->>AdminUI: OK
```
)MD";

static const char* SEQ_REPORT_MMD = R"MMD(sequenceDiagram
  autonumber
  participant InspApp as Inspector App (Mobile)
  participant Flag as FieldFlagGuard
  participant FE as Local Store/Sync Queue
  participant P2P as P2P/Relay
  participant Gate as API Gateway
  participant Queue as Submission Queue
  participant AdminUI as Admin UI (Web)
  participant DB as Admin DB
  participant Audit as Audit Log

  InspApp->>Flag: Capture notes + photos (media.hash, size limits)
  Flag-->>InspApp: Validated report (status=submitted)
  InspApp->>FE: Save Report (offline allowed)
  FE->>P2P: Try P2P delivery (idempotencyKey, contentHash)
  alt P2P success
    P2P-->>AdminUI: Report received
  else fallback relay
    FE->>Gate: POST /submit-report (scope: submit-report)
    Gate->>Queue: enqueue
    Queue-->>AdminUI: notify + payload
  end
  AdminUI->>DB: Upsert latest report & reset interval
  DB-->>AdminUI: OK
  AdminUI->>Audit: Append immutable entry (who/when/hash)
  Audit-->>AdminUI: OK
)MMD";

static const char* CLASS_MD = R"MD(# Sprint 2 – Class Diagram (Entities + Flags)

```mermaid
classDiagram
  class Assignment {
    +UUID id
    +UUID tenantId
    +ISODateTime createdAt
    +CreatedBy createdBy
    +UUID inspectorId
    +Target target
    +ISODateTime dueBy
    +AssignmentStatus status
    +string[] deliveryChannels
    +ISODateTime slaDeadlineAt [readOnly]
    +string priority
    +string notes
    +string idempotentKey
    +int sequence
    +map<string,int> vectorClock
    +bool auditEnabled=true
    +string signature?
    ---
    <<flags>>
    - id: immutable
    - tenantId: immutable, tenantScope:company
    - dueBy: inputType:datetime, required
    - deliveryChannels: defaultValue:["p2p","relay"]
  }

  class CreatedBy {
    +UUID userId
    +Role role  // Admin|ThirdPartyAdmin
    ---
    <<flags>> immutable, sensitivity:internal
  }

  class Target {
    +UUID buildingId
    +UUID[] extinguisherIds
    ---
    <<flags>> required
  }

  class InspectionReport {
    +UUID id
    +UUID tenantId
    +ISODateTime createdAt
    +CreatedByInspector createdBy
    +UUID assignmentId?
    +UUID extinguisherId
    +ReportStatus status
    +SubmissionMode submissionMode
    +Photo[] photos
    +float gaugeReadingPsi
    +bool obstruction
    +string notes
    +string mediaIntegrity
    +string anomalyFlag
    +bool deliverySlaMet
    +bool resetIntervalApplied
    +string signature?
    ---
    <<flags>>
    - photos: media.maxBytes, media.format, media.hash
    - gaugeReadingPsi: unit:psi, min/max
  }

  class CreatedByInspector {
    +UUID userId
    +string role // Inspector|ThirdPartyInspector
    ---
    <<flags>> immutable, sensitivity:internal
  }

  class Photo {
    +UUID mediaId
    +string contentType
    +int sizeBytes
    +string sha256
    +int width
    +int height
    +ISODateTime capturedAt
  }

  class Extinguisher {
    +UUID id
    +UUID tenantId
    +string serialNumber
    +string type  // ABC|CO2|Water|Foam|K-Class
    +Location location
    +int inspectionIntervalDays
    +ISODateTime nextDueAt [readOnly]
    +UUID lastInspectionReportId?
    +string status        // active|outOfService|retired|missing
    +string tamperSealStatus // unknown|intact|broken
    +string pressureStatus   // unknown|ok|low|high|damaged
    +ISODateTime createdAt
    +ISODateTime updatedAt
    ---
    <<flags>>
    - tenantId: tenantScope:company
    - serialNumber: regex, unique
    - nextDueAt: readOnly (derived)
  }

  class Location {
    +string address
    +string building
    +string floor
    +string room
    +string description
  }

  Assignment "1" --> "1" CreatedBy
  Assignment "1" --> "1" Target
  InspectionReport "1" --> "1" CreatedByInspector
  InspectionReport "1" --> "0..*" Photo
  Extinguisher "1" --> "1" Location
```
)MD";

static const char* CLASS_MMD = R"MMD(classDiagram
  class Assignment {
    +UUID id
    +UUID tenantId
    +ISODateTime createdAt
    +CreatedBy createdBy
    +UUID inspectorId
    +Target target
    +ISODateTime dueBy
    +AssignmentStatus status
    +string[] deliveryChannels
    +ISODateTime slaDeadlineAt [readOnly]
    +string priority
    +string notes
    +string idempotentKey
    +int sequence
    +map<string,int> vectorClock
    +bool auditEnabled=true
    +string signature?
    ---
    <<flags>>
    - id: immutable
    - tenantId: immutable, tenantScope:company
    - dueBy: inputType:datetime, required
    - deliveryChannels: defaultValue:["p2p","relay"]
  }

  class CreatedBy {
    +UUID userId
    +Role role  // Admin|ThirdPartyAdmin
    ---
    <<flags>> immutable, sensitivity:internal
  }

  class Target {
    +UUID buildingId
    +UUID[] extinguisherIds
    ---
    <<flags>> required
  }

  class InspectionReport {
    +UUID id
    +UUID tenantId
    +ISODateTime createdAt
    +CreatedByInspector createdBy
    +UUID assignmentId?
    +UUID extinguisherId
    +ReportStatus status
    +SubmissionMode submissionMode
    +Photo[] photos
    +float gaugeReadingPsi
    +bool obstruction
    +string notes
    +string mediaIntegrity
    +string anomalyFlag
    +bool deliverySlaMet
    +bool resetIntervalApplied
    +string signature?
    ---
    <<flags>>
    - photos: media.maxBytes, media.format, media.hash
    - gaugeReadingPsi: unit:psi, min/max
  }

  class CreatedByInspector {
    +UUID userId
    +string role // Inspector|ThirdPartyInspector
    ---
    <<flags>> immutable, sensitivity:internal
  }

  class Photo {
    +UUID mediaId
    +string contentType
    +int sizeBytes
    +string sha256
    +int width
    +int height
    +ISODateTime capturedAt
  }

  class Extinguisher {
    +UUID id
    +UUID tenantId
    +string serialNumber
    +string type  // ABC|CO2|Water|Foam|K-Class
    +Location location
    +int inspectionIntervalDays
    +ISODateTime nextDueAt [readOnly]
    +UUID lastInspectionReportId?
    +string status        // active|outOfService|retired|missing
    +string tamperSealStatus // unknown|intact|broken
    +string pressureStatus   // unknown|ok|low|high|damaged
    +ISODateTime createdAt
    +ISODateTime updatedAt
    ---
    <<flags>>
    - tenantId: tenantScope:company
    - serialNumber: regex, unique
    - nextDueAt: readOnly (derived)
  }

  class Location {
    +string address
    +string building
    +string floor
    +string room
    +string description
  }

  Assignment "1" --> "1" CreatedBy
  Assignment "1" --> "1" Target
  InspectionReport "1" --> "1" CreatedByInspector
  InspectionReport "1" --> "0..*" Photo
  Extinguisher "1" --> "1" Location
)MMD";

static const char* DOCS_README = R"MD(# Sprint 2 UML – Fire Watch App

This folder contains updated UML for Sprint 2, aligned with your Iteration 2 goals:

- Implement Web & Mobile UI and keep them in sync
- Identify and wire **field flags** to drive validation & UX
- Implement **Assignment generation/sending** (Admin → Inspector)
- Implement **Report creation/sending** (Inspector → Admin)
- Enable Admin control of static values (intervals, enums, templates)
- Run initial integration tests across local-first + P2P paths

## Files
- `sprint-2-activity.md` / `.mmd` — High-level activity flow for Sprint 2
- `sprint-2-usecase.md` / `.mmd` — Use case coverage, including Sprint 2 additions
- `sprint-2-components.md` / `.mmd` — Updated component boundaries & data flows
- `sprint-2-sequence-assignment.md` / `.mmd` — Sequence for Admin → Inspector assignment delivery
- `sprint-2-sequence-report.md` / `.mmd` — Sequence for Inspector → Admin report submission
- `sprint-2-class.md` / `.mmd` — Core entities with **flags** annotations

### Viewing
GitHub renders Mermaid **inside Markdown** automatically. For printable assets, our CI workflow renders `.mmd` → `.svg` into `docs/uml/`.
)MD";

static const char* TOP_README = R"MD(# Fire Watch UML Generator (C++)

This C++17 utility writes Sprint 2 Mermaid diagrams (.md and .mmd) into `docs/`. Use the included GitHub Actions workflow to render SVGs.

## Build
```bash
cmake -S . -B build && cmake --build build
```

## Run
```bash
./build/umlgen
```

## Output
- Markdown + Mermaid source: `docs/*.md` and `docs/*.mmd`
- CI renders SVG to `docs/uml/*.svg`
)MD";

static const char* CMAKE = R"CMAKE(cmake_minimum_required(VERSION 3.10)
project(umlgen LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
add_executable(umlgen src/umlgen.cpp)
)CMAKE";

int write_all(const std::vector<FileDef>& files){
    int errors = 0;
    for(const auto& f : files){
        fs::create_directories(fs::path(f.path).parent_path());
        std::ofstream o(f.path, std::ios::binary);
        if(!o){ std::cerr << "Failed to open: " << f.path << "\n"; ++errors; continue; }
        o << f.content; o.close();
        std::cout << "Wrote " << f.path << "\n";
    }
    return errors;
}

int main(){
    std::vector<FileDef> files = {
        {"docs/sprint-2-activity.md", ACTIVITY_MD},
        {"docs/sprint-2-usecase.md", USECASE_MD},
        {"docs/sprint-2-components.md", COMPONENTS_MD},
        {"docs/sprint-2-sequence-assignment.md", SEQ_ASSIGN_MD},
        {"docs/sprint-2-sequence-report.md", SEQ_REPORT_MD},
        {"docs/sprint-2-class.md", CLASS_MD},
        {"docs/sprint-2-activity.mmd", ACTIVITY_MMD},
        {"docs/sprint-2-usecase.mmd", USECASE_MMD},
        {"docs/sprint-2-components.mmd", COMPONENTS_MMD},
        {"docs/sprint-2-sequence-assignment.mmd", SEQ_ASSIGN_MMD},
        {"docs/sprint-2-sequence-report.mmd", SEQ_REPORT_MMD},
        {"docs/sprint-2-class.mmd", CLASS_MMD},
        {"README.md", TOP_README},
        {"docs/README.md", DOCS_README}
    };
    int rc = write_all(files);
    if(rc==0) std::cout << "All UML files written successfully.\n";
    return rc==0 ? 0 : 1;
}
