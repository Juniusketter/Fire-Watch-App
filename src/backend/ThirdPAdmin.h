#ifndef THIRDPADMIN_H
#define THIRDPADMIN_H

#include "User.h"
#include "Assignment.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;

/*
  Third Party Admin class - extends User class
  Created by Lilly Bowen
  Updated Sprint 2: Replaced cin/cout console I/O with parameter-based method.
  UI input is handled by AssignDialog in the Qt dashboard (dashboard.cpp)
  and by the Generate Assignment modal in the web UI (index.html).
  DB write is handled by the calling layer, not this class.
*/
class ThirdPAdmin : public User {
    private:
        string thirdPAdminID;

    public:
        // Default constructor
        ThirdPAdmin() : User(), thirdPAdminID("") {}

        // Parameterized constructor
        ThirdPAdmin(string u, string p, string c, string id) : User(u, p, c), thirdPAdminID(id) {}

        //getter and setter
        string getThirdPAdminID() { return thirdPAdminID; }
        void setThirdPAdminID(string id) { thirdPAdminID = id; }

        /*
           generateThirdPAssignment()
             Creates and returns an Assignment for a third-party admin.
             All required data is passed in as parameters — no console I/O.
         
           Parameters:
             extinguisherLocations - locations of extinguishers to inspect
             dueDate               - due date string (YYYY-MM-DD)
             invsList              - inspector IDs to assign
         
           Returns: populated Assignment object.
           The caller (Qt dashboard or Flask server) saves it to the DB.
        */ 
        Assignment generateThirdPAssignment(vector<string> extinguisherLocations, string dueDate, vector<string> invsList) {
            int numExtinguishers = extinguisherLocations.size();
            int numInvs = invsList.size();
            string id = getThirdPAdminID();

            Assignment newAssignment(numExtinguishers, numInvs, id, extinguisherLocations, dueDate, invsList);

            cout << "Third-party assignment generated: " << numExtinguishers << " extinguisher(s), " << numInvs << " inspector(s), due " << dueDate << "." << endl;

            return newAssignment;
        }
};

#endif // THIRDPADMIN_H
