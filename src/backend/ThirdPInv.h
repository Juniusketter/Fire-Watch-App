#ifndef THIRDPINV_H
#define THIRDPINV_H

#include "User.h"
#include <iostream>
#include <string>
using namespace std;

// Third Party Investigator class - extends User class
// Created by Lilly Bowen, March 4, 2026

class ThirdPInv : public User {
    private:
        string thirdPInvID;

    public:
        // Default constructor
        ThirdPInv() : User(), thirdPInvID("") {}

        // Parameterized constructor
        ThirdPInv(string u, string p, string c, string id)
            : User(u, p, c), thirdPInvID(id) {}

        string getThirdPInvID() {
            return thirdPInvID;
        }

        void setThirdPInvID(string id) {
            thirdPInvID = id;
        }

        void generateThirdPReport() {
            cout << "Third-party report generated successfully!" << endl;
        }
};

#endif // THIRDPINV_H
