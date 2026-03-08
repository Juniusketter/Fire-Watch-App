# Fire-Watch-App

A mobile-first fire extinguisher tracking application for professional safety inspectors.
Features a local-first architecture using C++ and SQLite for offline data management,
with peer-to-peer report sharing in compliance with NFPA standards.

---

## Project Structure

```
src/
  backend/         C++ class hierarchy (User, Admin, Inv, ThirdPAdmin, ThirdPInv)
  frontend/
    Fire-Watch-UI/ Qt6 desktop UI + HTML web UI
  database/        SQLite database (FireWatch.db)
```

---

## How to Test

### 1. HTML Frontend (No build needed — quickest test)

1. Open `src/frontend/Fire-Watch-UI/index.html` in any browser.
2. Try the demo credentials:
   - `admin` / `password123`
   - `inspector1` / `inv123`
   - `tpadmin` / `tpa123`
3. Invalid credentials will show an error message.

---

### 2. Backend C++ Classes (compile & run directly)

Requires: `g++` (any modern version)

```bash
cd src/backend
g++ -std=c++17 -o firewatch_test main.cpp
./firewatch_test
```

Expected output shows login success/failure, DB access tests, report generation, and logout for each user type.

---

### 3. Qt Desktop UI (requires Qt 6.5+)

Requires: Qt 6.5+, CMake 3.19+

```bash
cd src/frontend/Fire-Watch-UI
mkdir build && cd build
cmake ..
cmake --build .
./Fire-Watch-UI        # Linux/Mac
Fire-Watch-UI.exe      # Windows
```

Demo login: username `admin`, password `password123`

---

## What Was Fixed

| File | Issue | Fix |
|------|-------|-----|
| `User.cpp` / `Admin.cpp` etc. | Classes defined in `.cpp` files with no headers — subclasses couldn't inherit | Moved all class definitions to `.h` header files |
| All class files | Missing semicolons `;` at end of class braces | Added missing semicolons |
| All class files | No `#include <string>` — relied on transitive includes | Added explicit `#include <string>` |
| `User.cpp` | `login()` used infinite recursion on bad credentials | Replaced with bool return + caller handles retry |
| `Admin.cpp` | `accessDB()` used infinite recursion on wrong code | Replaced with bool return |
| All classes | No constructors — objects couldn't be initialized | Added default and parameterized constructors |
| `User.h` | No virtual destructor — memory leaks with polymorphism | Added `virtual ~User()` |
| `CMakeLists.txt` | `Qt::Core` / `Qt::Widgets` (alias) instead of `Qt6::` | Changed to `Qt6::Core` / `Qt6::Widgets` |
| `CMakeLists.txt` | Backend sources not included in Qt build | Added `BACKEND_SOURCES` and `include_directories` |
| `mainwindow.ui` | Generic widget names (`lineEdit`, `lineEdit_2`) | Renamed to `lineEditUsername`, `lineEditPassword`, `pushButtonLogin`, `labelMessage` |
| `mainwindow.ui` | Password field had placeholder text visible as text | Replaced with `placeholderText` property |
| `mainwindow.cpp` | Login button was never connected to any slot | Added `onLoginClicked()` slot wired to button and Enter key |
| `mainwindow.h` | No backend includes or slot declaration | Added includes and `private slots` |
| `index.html` | `fetch('/api/login')` fails when opened as a local file | Added demo credential mode; backend fetch preserved in comments |
| `index.html` | Button text color was black on green (low contrast) | Changed to white |

