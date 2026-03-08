#ifndef INV_H
#define INV_H

#include "User.h"
#include <iostream>
#include <string>
using namespace std;

// Investigator class - extends User class
// Created by Lilly Bowen, March 4, 2026

class Inv : public User {
    private:
        string invID;

    public:
        // Default constructor
        Inv() : User(), invID("") {}

        // Parameterized constructor
        Inv(string u, string p, string c, string id)
            : User(u, p, c), invID(id) {}

        string getInvID() {
            return invID;
        }

        void setInvID(string id) {
            invID = id;
        }

        void generateReport() {
            cout << "Report generated successfully!" << endl;
        }
};

#endif // INV_H
