#ifndef ADMIN_H
#define ADMIN_H

#include "User.h"
#include <iostream>
#include <string>
using namespace std;

// Admin class - extends User class
// Created by Lilly Bowen, March 3, 2026

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

        void setDBAccess(int access) {
            DBAccess = access;
        }

        int getDBAccess() {
            return DBAccess;
        }

        string getAdminID() {
            return adminID;
        }

        void setAdminID(string id) {
            adminID = id;
        }

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

        void changeDB(int access) {
            if (!accessDB(access)) return;

            int change;
            cout << "What would you like to do?\n  1. Add Extinguisher\n  2. Remove Extinguisher\n  3. Update Extinguisher\nChoice: ";
            cin >> change;

            switch (change) {
                case 1:
                    cout << "The extinguisher has been added successfully!" << endl;
                    break;
                case 2:
                    cout << "The extinguisher has been removed successfully!" << endl;
                    break;
                case 3: {
                    int update;
                    cout << "What would you like to update?\n  1. Extinguisher Type\n  2. Extinguisher Location\n  3. Inspection Interval\n  4. Add Report\nChoice: ";
                    cin >> update;
                    switch (update) {
                        case 1: cout << "The extinguisher type has been changed successfully!" << endl; break;
                        case 2: cout << "The extinguisher location has been changed successfully!" << endl; break;
                        case 3: cout << "The inspection interval has been changed successfully!" << endl; break;
                        case 4: cout << "The extinguisher report has been added successfully!" << endl; break;
                        default: cout << "Invalid option." << endl; break;
                    }
                    break;
                }
                default:
                    cout << "Invalid option." << endl;
                    break;
            }
        }

        void viewDB(int access) {
            if (!accessDB(access)) return;
            cout << "Here is the database." << endl;
        }

        void generateAssignment() {
            cout << "The assignment has been generated successfully!" << endl;
        }
};

#endif // ADMIN_H
