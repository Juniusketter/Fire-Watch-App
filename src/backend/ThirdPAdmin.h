#ifndef THIRDPADMIN_H
#define THIRDPADMIN_H

#include "User.h"
#include <iostream>
#include <string>
using namespace std;

// Third Party Admin class - extends User class
// Created by Lilly Bowen, March 4, 2026

class ThirdPAdmin : public User {
    private:
        string thirdPAdminID;

    public:
        // Default constructor
        ThirdPAdmin() : User(), thirdPAdminID("") {}

        // Parameterized constructor
        ThirdPAdmin(string u, string p, string c, string id)
            : User(u, p, c), thirdPAdminID(id) {}

        string getThirdPAdminID() {
            return thirdPAdminID;
        }

        void setThirdPAdminID(string id) {
            thirdPAdminID = id;
        }

        void generateThirdPAssignment() {
            // Assignment details: # extinguishers, inspector list, locations, due date
            cout << "Third-party assignment generated successfully!" << endl;
        }
};

#endif // THIRDPADMIN_H
