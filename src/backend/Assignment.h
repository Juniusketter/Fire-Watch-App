#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Assignment{
    private:
        int numExtinguishers;
        int numInvs;
        string identification;
        vector<string> extinguisherLocations;
        string dueDate;
        vector<string> invsList;
        string thirdPCompanyName;

    public:
        //default constructor
        Assignment() : numExtinguishers(0), numInvs(0), identification(""), extinguisherLocations({}), dueDate(""), invsList({}) {}
        //parametized constructors
        //if admin is using investigators
        Assignment(int numFEs, int invs, string id, vector<string> feLocations, string date, vector<string> listInvs) {
            numExtinguishers = numFEs;
            numInvs = invs;
            identification = id;
            extinguisherLocations = feLocations;
            dueDate = date;
            invsList = listInvs;
        }
        //if admin is using third party company
        Assignment(int numFEs, int invs, string id, vector<string> feLocations, string date, string thirdPComp) {
            numExtinguishers = numFEs;
            numInvs = invs;
            identification = id;
            extinguisherLocations = feLocations;
            dueDate = date;
            thirdPCompanyName = thirdPComp;
        }


        //getters and setters
        void setNumExtinguishers(int num) {
            numExtinguishers = num;
        }
        int getNumExtinguishers() {
            return numExtinguishers;
        }
        void setNumInvestigators(int num) {
            numInvs = num;
        }
        int getNumInvestigators() {
            return numInvs;
        }
        void setCompanyName(string name) {
            adminName = name;
        }
        string getCompanyName() {
            return adminName;
        }
        void setExtinguisherLocations(vector<string> locations) {
            extinguisherLocations = locations;
        }
        vector<string> getExtinguisherLocations() {
            return extinguisherLocations;
        }
        void setDueDate(string date) {
            dueDate = date;
        }
        string getDueDate() {
            return dueDate;
        }
        void setInvsList(vector<string> list) {
            invsList = list;
        }
        vector<string> getInvsList() {
            return invsList;
        }
        void setExtinguisherList(vector<string> list) {
            extinguisherList = list;
        }
        vector<string> getExtinguisherList() {
            return extinguisherList;
        }

        void sendAssignment(){
            //send assignment code
            //from admin to investigator or third party company
            cout << "Assignment sent successfully!" << endl;
        }

};
