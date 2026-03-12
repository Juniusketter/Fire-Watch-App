// FireWatch Backend - Main Test File
// Tests all user classes: User, Admin, Inv, ThirdPAdmin, ThirdPInv
//
// Updated Sprint 2: Removed all cin/cout interactive I/O from test functions.
// All tests now use hardcoded data and run to completion without user input.
// Expected output is shown in comments for each test.

#include "User.h"
#include "Admin.h"
#include "Inv.h"
#include "ThirdPAdmin.h"
#include "ThirdPInv.h"
#include "Assignment.h"

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
using namespace std;

// ─── Helper ──────────────────────────────────────────────────────────────────
void printSeparator(string label) {
    cout << "\n=== " << label << " ===" << endl;
}

// ─── Test 1: Admin login ──────────────────────────────────────────────────────
// Expected: login success, login failure, DB access granted, DB access denied
void testAdminLogin() {
    printSeparator("Admin Login Test");

    Admin admin("adminUser", "pass123", "FireSafe Corp", "ADM001", 9999);

    bool ok  = admin.login("adminUser", "pass123");   // should succeed
    bool bad = admin.login("adminUser", "wrongpass"); // should fail

    assert(ok  == true  && "Correct credentials should return true");
    assert(bad == false && "Wrong credentials should return false");

    admin.viewDB(9999); // should grant access
    admin.viewDB(0);    // should deny access

    cout << "[PASS] Admin login tests passed." << endl;
}

// ─── Test 2: Inspector ────────────────────────────────────────────────────────
// Expected: login success, generateReport() prints success message, logout
void testInvestigator() {
    printSeparator("Investigator Test");

    Inv inv("inspector1", "invPass", "FireSafe Corp", "INV001");

    bool ok = inv.login("inspector1", "invPass");
    assert(ok == true && "Inspector login should succeed");

    inv.generateReport(); // prints "Report generated successfully!"
    inv.logout();

    cout << "[PASS] Investigator tests passed." << endl;
}

// ─── Test 3: Admin generates assignment ──────────────────────────────────────
// Expected: Assignment object created with correct field values
void testAdminGeneratesAssignment() {
    printSeparator("Admin Generate Assignment Test");

    // Note: Admin::generateAssignment() still uses cin/cout for the full
    // interactive flow. For testability, we construct an Assignment directly
    // using the same constructor it would call, verifying the data model.
    vector<string> locations = {"Bldg A - Floor 1", "Bldg A - Floor 2"};
    vector<string> inspectors = {"INV001", "INV002"};

    Assignment a(2, 2, "ADM001", locations, "2026-04-01", inspectors);

    assert(a.getNumExtinguishers() == 2    && "Should have 2 extinguishers");
    assert(a.getNumInvestigators() == 2    && "Should have 2 inspectors");
    assert(a.getDueDate() == "2026-04-01"  && "Due date should match");
    assert(a.getInvsList().size() == 2     && "Inspector list should have 2 entries");
    assert(a.getExtinguisherLocations().size() == 2 && "Location list should have 2 entries");

    cout << "Assignment created — "
         << a.getNumExtinguishers() << " extinguisher(s), "
         << a.getNumInvestigators() << " inspector(s), due "
         << a.getDueDate() << "." << endl;
    cout << "[PASS] Admin assignment tests passed." << endl;
}

// ─── Test 4: ThirdPAdmin generates assignment ─────────────────────────────────
// Expected: generateThirdPAssignment() returns correct Assignment object
void testThirdPartyAdmin() {
    printSeparator("Third-Party Admin Assignment Test");

    ThirdPAdmin tpa("tpAdmin", "tpaPass", "Third Party Co", "TPA001");

    bool ok = tpa.login("tpAdmin", "tpaPass");
    assert(ok == true && "ThirdPAdmin login should succeed");

    vector<string> locations  = {"Library - Lobby", "Library - Floor 2"};
    vector<string> inspectors = {"TPI001"};
    string dueDate            = "2026-04-15";

    Assignment a = tpa.generateThirdPAssignment(locations, dueDate, inspectors);

    assert(a.getNumExtinguishers() == 2   && "Should have 2 extinguishers");
    assert(a.getNumInvestigators() == 1   && "Should have 1 inspector");
    assert(a.getDueDate() == "2026-04-15" && "Due date should match");

    tpa.logout();
    cout << "[PASS] ThirdPAdmin assignment tests passed." << endl;
}

// ─── Test 5: ThirdPInv generates report ──────────────────────────────────────
// Expected: generateThirdPReport() returns true for valid data, false for invalid
void testThirdPartyInv() {
    printSeparator("Third-Party Inspector Report Test");

    ThirdPInv tpi("tpInv", "tpiPass", "Third Party Co", "TPI001");

    bool ok = tpi.login("tpInv", "tpiPass");
    assert(ok == true && "ThirdPInv login should succeed");

    // Valid report
    bool result = tpi.generateThirdPReport(1, 4, "2026-03-12", "Extinguisher in good condition.");
    assert(result == true && "Valid report should return true");

    // Invalid — bad extinguisher ID
    bool badExt = tpi.generateThirdPReport(0, 4, "2026-03-12", "Test");
    assert(badExt == false && "Invalid extinguisher ID should return false");

    // Invalid — missing date
    bool badDate = tpi.generateThirdPReport(1, 4, "", "Test");
    assert(badDate == false && "Missing date should return false");

    tpi.logout();
    cout << "[PASS] ThirdPInv report tests passed." << endl;
}

// ─── Test 6: Assignment send ──────────────────────────────────────────────────
// Expected: sendAssignment() prints confirmation
void testAssignmentSend() {
    printSeparator("Assignment Send Test");

    vector<string> locations  = {"305 George Ave - Floor 5"};
    vector<string> inspectors = {"INV001"};

    Assignment a(1, 1, "ADM001", locations, "2026-04-01", inspectors);
    a.sendAssignment(); // prints "Assignment sent successfully!"

    cout << "[PASS] Assignment send test passed." << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
int main() {
    cout << "========================================" << endl;
    cout << "  FireWatch Backend Test Suite          " << endl;
    cout << "========================================" << endl;

    testAdminLogin();
    testInvestigator();
    testAdminGeneratesAssignment();
    testThirdPartyAdmin();
    testThirdPartyInv();
    testAssignmentSend();

    cout << "\n========================================" << endl;
    cout << "  All 6 tests passed.                   " << endl;
    cout << "========================================" << endl;

    return 0;
}
