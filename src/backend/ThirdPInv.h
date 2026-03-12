#ifndef THIRDPINV_H
#define THIRDPINV_H

#include "User.h"
#include <iostream>
#include <string>
using namespace std;

// Third Party Investigator class - extends User class
// Created by Lilly Bowen, March 4, 2026
// Updated Sprint 2: Implemented generateThirdPReport() with parameters.
// Previously a stub that only printed a success message.
// UI input is handled by ReportDialog in the Qt dashboard (dashboard.cpp)
// and by the Submit Report modal in the web UI (index.html).
// DB write is handled by the calling layer, not this class.

class ThirdPInv : public User {
    private:
        string thirdPInvID;

    public:
        // Default constructor
        ThirdPInv() : User(), thirdPInvID("") {}

        // Parameterized constructor
        ThirdPInv(string u, string p, string c, string id)
            : User(u, p, c), thirdPInvID(id) {}

        string getThirdPInvID() { return thirdPInvID; }
        void   setThirdPInvID(string id) { thirdPInvID = id; }

        // ─────────────────────────────────────────────────────────────────
        //  generateThirdPReport()
        //
        //  Records an inspection report for a third-party inspector.
        //  All required data is passed in as parameters — no console I/O.
        //  Mirrors Inv::generateReport() but includes the assignment ID
        //  so the calling layer can mark the assignment complete.
        //
        //  Parameters:
        //    extinguisherId - ID of the extinguisher inspected
        //    assignmentId   - ID of the assignment being fulfilled
        //    inspectionDate - date of inspection (YYYY-MM-DD)
        //    notes          - inspection notes or findings
        //
        //  Returns: true if report data is valid, false otherwise.
        //  The caller (Qt dashboard or Flask server) saves to the DB
        //  and updates the assignment status to "Inspection Complete".
        // ─────────────────────────────────────────────────────────────────
        bool generateThirdPReport(
            int extinguisherId,
            int assignmentId,
            string inspectionDate,
            string notes)
        {
            if (extinguisherId <= 0 || assignmentId <= 0) {
                cout << "Error: Invalid extinguisher or assignment ID." << endl;
                return false;
            }
            if (inspectionDate.empty()) {
                cout << "Error: Inspection date is required." << endl;
                return false;
            }

            cout << "Third-party report generated: "
                 << "Ext #" << extinguisherId
                 << ", Assignment #" << assignmentId
                 << ", Date: " << inspectionDate << "." << endl;

            return true;
        }
};

#endif // THIRDPINV_H
