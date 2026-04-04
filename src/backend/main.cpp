// FireWatch Backend - Main Test File
// Tests all user classes: User, Admin, Inv, ThirdPAdmin, ThirdPInv
//
// Sprint 2: All tests use hardcoded data — no cin calls, runs to completion.

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

void printSeparator(string label) {
    cout << "\n=== " << label << " ===" << endl;
}

// ─── Test 1: Admin login ──────────────────────────────────────────────────────
void testAdminLogin() {
    printSeparator("Admin Login Test");

    Admin admin("adminUser", "pass123", "FireSafe Corp", "ADM001", 9999);

    bool ok  = admin.login("adminUser", "pass123");
    bool bad = admin.login("adminUser", "wrongpass");

    assert(ok  == true);
    assert(bad == false);

    admin.viewDB(9999);  // granted
    admin.viewDB(0);     // denied

    cout << "[PASS] Admin login tests passed." << endl;
}

// ─── Test 2: Admin changeDB ───────────────────────────────────────────────────
void testAdminChangeDB() {
    printSeparator("Admin changeDB Test");

    Admin admin("adminUser", "pass123", "FireSafe Corp", "ADM001", 9999);

    bool added   = admin.changeDB(9999, 1);       // Add extinguisher
    bool removed = admin.changeDB(9999, 2);       // Remove extinguisher
    bool updated = admin.changeDB(9999, 3, 2);    // Update location
    bool denied  = admin.changeDB(0,    1);       // Wrong access code
    bool invalid = admin.changeDB(9999, 99);      // Invalid action

    assert(added   == true);
    assert(removed == true);
    assert(updated == true);
    assert(denied  == false);
    assert(invalid == false);

    cout << "[PASS] Admin changeDB tests passed." << endl;
}

// ─── Test 3: Admin generateAssignment (internal inspectors) ──────────────────
void testAdminGeneratesAssignmentInternal() {
    printSeparator("Admin Generate Assignment (Internal) Test");

    Admin admin("adminUser", "pass123", "FireSafe Corp", "ADM001", 9999);

    vector<string> locations  = {"Bldg A - Floor 1", "Bldg A - Floor 2"};
    vector<string> inspectors = {"INV001", "INV002"};

    Assignment a = admin.generateAssignment(locations, "2026-04-01", inspectors);

    assert(a.getNumExtinguishers() == 2);
    assert(a.getNumInvestigators() == 2);
    assert(a.getDueDate() == "2026-04-01");
    assert(a.getInvsList().size() == 2);

    cout << "[PASS] Admin internal assignment tests passed." << endl;
}

// ─── Test 4: Admin generateAssignment (third party company) ──────────────────
void testAdminGeneratesAssignmentThirdParty() {
    printSeparator("Admin Generate Assignment (Third Party) Test");

    Admin admin("adminUser", "pass123", "FireSafe Corp", "ADM001", 9999);

    vector<string> locations = {"Library - Lobby"};
    vector<string> noInvs    = {};

    Assignment a = admin.generateAssignment(locations, "2026-04-15", noInvs, "SafeGuard Co");

    assert(a.getNumExtinguishers() == 1);
    assert(a.getDueDate() == "2026-04-15");

    cout << "[PASS] Admin third-party assignment tests passed." << endl;
}

// ─── Test 5: Inspector generateReport ────────────────────────────────────────
void testInvestigator() {
    printSeparator("Investigator Report Test");

    Inv inv("inspector1", "invPass", "FireSafe Corp", "INV001");

    bool ok = inv.login("inspector1", "invPass");
    assert(ok == true);

    bool valid   = inv.generateReport(1, 3, "2026-03-12", "Extinguisher in good condition.");
    bool badExt  = inv.generateReport(0, 3, "2026-03-12", "Test");
    bool badDate = inv.generateReport(1, 3, "",            "Test");

    assert(valid   == true);
    assert(badExt  == false);
    assert(badDate == false);

    inv.logout();
    cout << "[PASS] Investigator report tests passed." << endl;
}

// ─── Test 6: ThirdPAdmin generateAssignment ───────────────────────────────────
void testThirdPartyAdmin() {
    printSeparator("Third-Party Admin Assignment Test");

    ThirdPAdmin tpa("tpAdmin", "tpaPass", "Third Party Co", "TPA001");

    bool ok = tpa.login("tpAdmin", "tpaPass");
    assert(ok == true);

    vector<string> locations  = {"Library - Lobby", "Library - Floor 2"};
    vector<string> inspectors = {"TPI001"};

    Assignment a = tpa.generateThirdPAssignment(locations, "2026-04-15", inspectors);

    assert(a.getNumExtinguishers() == 2);
    assert(a.getNumInvestigators() == 1);
    assert(a.getDueDate() == "2026-04-15");

    tpa.logout();
    cout << "[PASS] ThirdPAdmin assignment tests passed." << endl;
}

// ─── Test 7: ThirdPInv generateReport ────────────────────────────────────────
void testThirdPartyInv() {
    printSeparator("Third-Party Inspector Report Test");

    ThirdPInv tpi("tpInv", "tpiPass", "Third Party Co", "TPI001");

    bool ok = tpi.login("tpInv", "tpiPass");
    assert(ok == true);

    bool valid   = tpi.generateThirdPReport(1, 4, "2026-03-12", "All clear.");
    bool badExt  = tpi.generateThirdPReport(0, 4, "2026-03-12", "Test");
    bool badDate = tpi.generateThirdPReport(1, 4, "",            "Test");

    assert(valid   == true);
    assert(badExt  == false);
    assert(badDate == false);

    tpi.logout();
    cout << "[PASS] ThirdPInv report tests passed." << endl;
}

// ─── Test 8: Assignment send ──────────────────────────────────────────────────
void testAssignmentSend() {
    printSeparator("Assignment Send Test");

    vector<string> locations  = {"305 George Ave - Floor 5"};
    vector<string> inspectors = {"INV001"};

    Assignment a(1, 1, "ADM001", locations, "2026-04-01", inspectors);
    a.sendAssignment();

    cout << "[PASS] Assignment send test passed." << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
int main() {
    cout << "========================================" << endl;
    cout << "  FireWatch Backend Test Suite          " << endl;
    cout << "========================================" << endl;

    testAdminLogin();
    testAdminChangeDB();
    testAdminGeneratesAssignmentInternal();
    testAdminGeneratesAssignmentThirdParty();
    testInvestigator();
    testThirdPartyAdmin();
    testThirdPartyInv();
    testAssignmentSend();

    cout << "\n========================================" << endl;
    cout << "  All 8 tests passed.                   " << endl;
    cout << "========================================" << endl;

    return 0;
}
