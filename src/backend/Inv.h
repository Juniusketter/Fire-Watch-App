#ifndef INV_H
#define INV_H

#include "User.h"
#include <iostream>
#include <string>
using namespace std;

/* 
   Investigator class - extends User class
   Created by Lilly Bowen
   Updated Sprint 2: Implemented generateReport() with parameters.
   Previously a stub that only printed a success message.
   UI input is handled by ReportDialog in the Qt dashboard (dashboard.cpp)
   and by the Submit Report modal in the web UI (index.html).
   DB write is handled by the calling layer, not this class.
*/
class Inv : public User {
    private:
        string invID;

    public:
        // Default constructor
        Inv() : User(), invID("") {}

        // Parameterized constructor
        Inv(string u, string p, string c, string id) : User(u, p, c), invID(id) {}

        //getter and setter
        string getInvID() { return invID; }
        void   setInvID(string id) { invID = id; }

        /* 
            generateReport()
              Records an inspection report for an internal inspector.
              All required data is passed in as parameters — no console I/O.
              The assignment is automatically marked "Inspection Complete"
              by the calling layer after this returns true.
        
            Parameters:
              extinguisherId - ID of the extinguisher inspected
              assignmentId   - ID of the assignment being fulfilled
              inspectionDate - date of inspection (YYYY-MM-DD)
              notes          - inspection notes or findings
        
            Returns: true if report data is valid, false otherwise.
            The caller (Qt dashboard or Flask server) saves to the DB.
        */ 
        bool generateReport(int extinguisherId, int assignmentId, string inspectionDate, string notes)
        {
            if (extinguisherId <= 0 || assignmentId <= 0) {
                cout << "Error: Invalid extinguisher or assignment ID." << endl;
                return false;
            }
            if (inspectionDate.empty()) {
                cout << "Error: Inspection date is required." << endl;
                return false;
            }

            cout << "Report generated: " << "Ext #" << extinguisherId << ", Assignment #" << assignmentId << ", Date: " << inspectionDate << "." << endl;

            return true;
        }
};

#endif // INV_H
