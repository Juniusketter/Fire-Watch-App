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
        //change and view db functions
        void changeDB(int access) {
            if (!accessDB(access)) return;

            int change;
            cout << "What would you like to do?\n  1. Add Extinguisher\n  2. Remove Extinguisher\n  3. Update Extinguisher\nChoice: ";
            cin >> change;

            switch (change) {
                case 1:
                    cout << "The extinguisher has been added successfully!" << endl; break;
                case 2:
                    cout << "The extinguisher has been removed successfully!" << endl; break;
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

        //I know that a lot of this code is going to need to be changed to correctly integrate with the front end code, but this is the general idea
        Assignment generateAssignment() {
            //declare variables used in all assignments
            int choice;
            int numInvs;
            int numExtinguishers;
            string dueDate;
            vector<string> extinguisherLocations;
            string id = getAdminID();

            //determine recipient of the assignment
            cout<< "Will you be sending this to your own investigators, or a third party company? (1. My own investigators, 2. Third party company)" << endl;
            cin>> choice;

            //gather necessary information for the assignment
            cout << "How many investigators will be assigned to this task?" << endl;
            cin >> numInvs;

            cout << "How many extinguishers need to be inspected?" << endl;
            cin >> numExtinguishers;

            cout << "When is the due date for this assignment?" << endl;
            cin >> dueDate;
            
            for(int i = 0; i < numExtinguishers; i++){
                string location;
                if(i == 0){
                    cout << "Please enter the " << i+1 << "st extinguisher location. " << endl;
                }
                else if(i == 1){
                    cout << "Please enter the " << i+1 << "nd extinguisher location. " << endl;
                }
                else if(i == 2){
                    cout << "Please enter the " << i+1 << "rd extinguisher location. " << endl;
                }
                else{
                    cout << "Please enter the " << i+1 << "th extinguisher location. " << endl;
                }
                cin >> location;
                extinguisherLocations.push_back(location);
            }

            //gather information specific to the recipient of the assignment and generate the assignment
            switch(choice){
                case 1: {
                    vector<string> invsList;
                    for(int i = 0; i < numInvs; i++){
                        string invs;
                        if(i == 0){
                            cout << "Please enter the " << i+1 << "st investigator ID you want to assign. " << endl;
                        }
                        else if(i == 1){
                            cout << "Please enter the " << i+1 << "nd investigator ID you want to assign. " << endl;
                        }
                        else if(i == 2){
                            cout << "Please enter the " << i+1 << "rd investigator ID you want to assign. " << endl;
                        }
                        else{
                            cout << "Please enter the " << i+1 << "th investigator ID you want to assign. " << endl;
                        }
                        cin >> invs;
                        invsList.push_back(invs);
                    }

                    //create assignment
                    Assignment newAssignment(numExtinguishers, numInvs, id, extinguisherLocations, dueDate, invsList);
                    cout << "Assignment generated successfully!" << endl;
                    return newAssignment;
                    break;
                }
                case 2: {
                    string thirdPCompanyName;
                    cout << "Please enter the name of the third party company you want to assign this task to." << endl;
                    cin >> thirdPCompanyName;
                    
                    //create assignment
                    Assignment newAssignment(numExtinguishers, numInvs, id, extinguisherLocations, dueDate, thirdPCompanyName);
                    cout << "Assignment generated successfully!" << endl;
                    return newAssignment;
                    break;
                }
                default:
                    cout << "Invalid option. Returning default assignment." << endl;
                    return Assignment();
            }
        }
};

#endif // ADMIN_H
