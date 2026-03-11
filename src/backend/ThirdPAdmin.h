#ifndef THIRDPADMIN_H
#define THIRDPADMIN_H

#include "User.h"
#include "Assignment.h"
#include <iostream>
#include <string>
#include <vector>
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

        //I know that a lot of this code is going to need to be changed to correctly integrate with the front end code, but this is the general idea
        Assignment generateThirdPAssignment(){
            //declare variables used in assignment
            int choice;
            int numInvs;
            int numExtinguishers;
            string dueDate;
            vector<string> extinguisherLocations;
            vector<string> invsList;
            string id = getThirdPAdminID();

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
            
            Assignment newAssignment(numExtinguishers, numInvs, id, extinguisherLocations, dueDate, invsList);
            cout << "Assignment generated successfully!" << endl;
            return newAssignment;
        }
};

#endif // THIRDPADMIN_H
