#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Assignment {
    private:
        int numExtinguishers;
        int numInvs;
        string identification;
        vector<string> extinguisherLocations;
        vector<string> extinguisherList;     // FIX: was missing, caused compile error
        string dueDate;
        vector<string> invsList;
        string thirdPCompanyName;
        string adminName;                    // FIX: was missing, caused compile error

    public:
        // Default constructor
        Assignment() : numExtinguishers(0), numInvs(0), identification(""),
                       extinguisherLocations({}), dueDate(""), invsList({}) {}

        // Parameterized constructor — admin using investigators
        Assignment(int numFEs, int invs, string id, vector<string> feLocations,
                   string date, vector<string> listInvs) {
            numExtinguishers      = numFEs;
            numInvs               = invs;
            identification        = id;
            extinguisherLocations = feLocations;
            dueDate               = date;
            invsList              = listInvs;
        }

        // Parameterized constructor — admin using third party company
        Assignment(int numFEs, int invs, string id, vector<string> feLocations,
                   string date, string thirdPComp) {
            numExtinguishers      = numFEs;
            numInvs               = invs;
            identification        = id;
            extinguisherLocations = feLocations;
            dueDate               = date;
            thirdPCompanyName     = thirdPComp;
        }

        // Getters and setters
        void setNumExtinguishers(int num)       { numExtinguishers = num; }
        int  getNumExtinguishers()              { return numExtinguishers; }

        void setNumInvestigators(int num)       { numInvs = num; }
        int  getNumInvestigators()              { return numInvs; }

        void   setCompanyName(string name)      { adminName = name; }
        string getCompanyName()                 { return adminName; }

        void           setExtinguisherLocations(vector<string> locations) { extinguisherLocations = locations; }
        vector<string> getExtinguisherLocations()                         { return extinguisherLocations; }

        void   setDueDate(string date)          { dueDate = date; }
        string getDueDate()                     { return dueDate; }

        void           setInvsList(vector<string> list) { invsList = list; }
        vector<string> getInvsList()                    { return invsList; }

        void           setExtinguisherList(vector<string> list) { extinguisherList = list; }
        vector<string> getExtinguisherList()                    { return extinguisherList; }

        void sendAssignment() {
            // Send assignment from admin to investigator or third party company
            cout << "Assignment sent successfully!" << endl;
        }
};

#endif // ASSIGNMENT_H
