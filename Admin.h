// Admin.h - Full implementation backend for edit/delete users
#pragma once
#include <string>
#include <sqlite3.h>
class Admin{
public:
 std::string userId;
 std::string companyId;
 Admin(std::string u,std::string c):userId(u),companyId(c){}
 bool editUser(sqlite3*db,const std::string&uid,const std::string&email,const std::string&name,const std::string&role,std::string*err);
 bool deleteUser(sqlite3*db,const std::string&uid,std::string*err);
 void audit(sqlite3*db,const std::string&entity,const std::string&id,const std::string&action);
};
#ifndef ADMIN_H
#define ADMIN_H

#include "User.h"
#include "Assignment.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Admin class - extends User class
// Created by Lilly Bowen, March 3, 2026
// Updated Sprint 2: Replaced cin/cout console I/O in generateAssignment()
// and changeDB() with parameter-based methods.
// UI input is handled by AssignDialog + ExtDialog in the Qt dashboard
// and by the web UI modals (index.html). DB writes are handled by the
// calling layer (dashboard.cpp / server.py), not this class.

class Admin : public User {
    private:
        string adminID;
        int DBAccess;

    public:
        // Default constructor
        Admin() : User(), adminID(""), DBAccess(0) {}

        // Parameterized constructor
        Admin(string u, string p, string c, string id, int access)
            : User(u, p, c), adminID(id), DBAccess(access) {}

        void   setDBAccess(int access) { DBAccess = access; }
        int    getDBAccess()           { return DBAccess; }
        string getAdminID()            { return adminID; }
        void   setAdminID(string id)   { adminID = id; }

        // Returns true if access code matches
        bool accessDB(int access) {
            if (DBAccess == access) {
                cout << "Access granted. You can now view and edit the database." << endl;
                return true;
            } else {
                cout << "You do not have access to the database. Please check your access code." << endl;
                return false;
            }
        }

        // ─────────────────────────────────────────────────────────────────
        //  changeDB()
        //
        //  Modifies an extinguisher record in the database.
        //  All required data is passed in as parameters — no console I/O.
        //  The Qt dashboard (ExtDialog) handles UI input and calls this
        //  method with validated data, or writes directly via QSqlQuery.
        //
        //  Parameters:
        //    access   - admin access code for authorization
        //    action   - 1=Add, 2=Remove, 3=Update
        //    field    - (for action=3) 1=Type, 2=Location, 3=Interval, 4=Report
        //
        //  Returns: true if the action was authorized and valid.
        // ─────────────────────────────────────────────────────────────────
        bool changeDB(int access, int action, int field = 0) {
            if (!accessDB(access)) return false;

            switch (action) {
                case 1:
                    cout << "The extinguisher has been added successfully!" << endl;
                    return true;
                case 2:
                    cout << "The extinguisher has been removed successfully!" << endl;
                    return true;
                case 3:
                    switch (field) {
                        case 1: cout << "The extinguisher type has been changed successfully!"     << endl; return true;
                        case 2: cout << "The extinguisher location has been changed successfully!" << endl; return true;
                        case 3: cout << "The inspection interval has been changed successfully!"   << endl; return true;
                        case 4: cout << "The extinguisher report has been added successfully!"     << endl; return true;
                        default: cout << "Invalid field option." << endl; return false;
                    }
                default:
                    cout << "Invalid action option." << endl;
                    return false;
            }
        }

        void viewDB(int access) {
            if (!accessDB(access)) return;
            cout << "Here is the database." << endl;
        }

        // ─────────────────────────────────────────────────────────────────
        //  generateAssignment()
        //
        //  Creates and returns an Assignment for an admin.
        //  All required data is passed in as parameters — no console I/O.
        //  Supports two recipient types:
        //    - invsList populated  → assign to internal inspectors
        //    - thirdPCompanyName   → assign to a third-party company
        //
        //  Parameters:
        //    extinguisherLocations - locations of extinguishers to inspect
        //    dueDate               - due date string (YYYY-MM-DD)
        //    invsList              - inspector IDs (empty if third party)
        //    thirdPCompanyName     - third party company name (empty if internal)
        //
        //  Returns: populated Assignment object.
        //  The caller (Qt dashboard or Flask server) saves it to the DB.
        // ─────────────────────────────────────────────────────────────────
        Assignment generateAssignment(
            vector<string> extinguisherLocations,
            string dueDate,
            vector<string> invsList,
            string thirdPCompanyName = "")
        {
            int numExtinguishers = extinguisherLocations.size();
            int numInvs          = invsList.size();
            string id            = getAdminID();

            if (!thirdPCompanyName.empty()) {
                // Assign to third-party company
                Assignment newAssignment(
                    numExtinguishers, numInvs, id,
                    extinguisherLocations, dueDate, thirdPCompanyName
                );
                cout << "Assignment generated for third-party company: "
                     << thirdPCompanyName << ", "
                     << numExtinguishers << " extinguisher(s), due " << dueDate << "." << endl;
                return newAssignment;
            } else {
                // Assign to internal inspectors
                Assignment newAssignment(
                    numExtinguishers, numInvs, id,
                    extinguisherLocations, dueDate, invsList
                );
                cout << "Assignment generated: "
                     << numExtinguishers << " extinguisher(s), "
                     << numInvs << " inspector(s), due " << dueDate << "." << endl;
                return newAssignment;
            }
        }

        // ─────────────────────────────────────────────────────────────────
        //  Export/Print functions moved to ExportDialog (exportdialog.h)
        //  in the UI layer. Original code by Lillian Bowen — Sprint 3.
        //  Backend classes should not depend on Qt UI modules.
        // ─────────────────────────────────────────────────────────────────


};

#endif // ADMIN_H
