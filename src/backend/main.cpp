// FireWatch Backend - Main Test File
// Tests all user classes: User, Admin, Inv, ThirdPAdmin, ThirdPInv

#include "User.h"
#include "Admin.h"
#include "Inv.h"
#include "ThirdPAdmin.h"
#include "ThirdPInv.h"

#include <iostream>
#include <string>
using namespace std;

void testAdminLogin() {
    cout << "\n=== Admin Login Test ===" << endl;
    Admin admin("adminUser", "pass123", "FireSafe Corp", "ADM001", 9999);

    // Correct credentials
    admin.login("adminUser", "pass123");

    // Wrong credentials
    admin.login("adminUser", "wrongpass");

    // DB Access
    admin.viewDB(9999);
    admin.viewDB(0000); // wrong access code
}

void testInvestigator() {
    cout << "\n=== Investigator Test ===" << endl;
    Inv inv("inspector1", "invPass", "FireSafe Corp", "INV001");
    inv.login("inspector1", "invPass");
    inv.generateReport();
    inv.logout();
}

void testThirdParty() {
    cout << "\n=== Third-Party Admin & Investigator Test ===" << endl;

    ThirdPAdmin tpa("tpAdmin", "tpaPass", "Third Party Co", "TPA001");
    tpa.login("tpAdmin", "tpaPass");
    tpa.generateThirdPAssignment();
    tpa.logout();

    ThirdPInv tpi("tpInv", "tpiPass", "Third Party Co", "TPI001");
    tpi.login("tpInv", "tpiPass");
    tpi.generateThirdPReport();
    tpi.logout();
}

int main() {
    cout << "===== FireWatch Backend Test Suite =====" << endl;

    testAdminLogin();
    testInvestigator();
    testThirdParty();

    cout << "\n===== All Tests Completed =====" << endl;
    return 0;
}
