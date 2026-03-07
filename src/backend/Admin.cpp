#include <iostream>
using namespace std;

//started by Lilly Bowen on March 3, 2026
//creates Admin class which extends User class

//added by Lilly on March 6
//requirements to be tested throughout process:
//Admin object is created
//functional getters & setters
//ensure functions from parent class work
//ensure db functions (access, change, view) work
//ensure assignment generation and sending works\
//ensure that the admin can view reports (not sure if this will be included here or in a different class)

class Admin : public User{
    private:
        string adminID;
        int DBAccess;

    public :
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

        void accessDB(int access){
            if(DBAccess == access){
                //connect to database
                cout << "Access granted. You can now view and edit the database." << endl;
            }
            else{
                cout << "You do not have access to the database. Please try the code again or contact the owner of this database." << endl;
                accessDB(access);
            }
        }

        void changeDB(int access){
            accessDB(access);
            int change;
            cout << "What would you like to do? (1. Add Extinguisher, 2. Remove Extinguisher, 3. Update Extinguisher)" << endl;
            cin >> change;
            switch(change){
                case 1:
                    //add extinguisher code
                    cout << "The extinguisher has been added successfully!" << endl;
                    break;
                case 2:
                    //remove extinguisher code
                    cout << "The extinguisher has been removed successfully!" << endl;
                    break;
                case 3:
                    //update extinguisher code
                    int update;
                    cout << "What would you like to update? (1. Extinguisher Type, 2. Extinguisher Location, 3. Inspection Interval, 4. Add Report)" << endl;
                    cin >> update;
                    switch(update){
                        case 1:
                            //update extinguisher type code
                            cout << "The extinguisher type has been changed successfully!" << endl;
                            break;
                        case 2:
                            //update extinguisher location code
                            cout << "The extinguisher location has been changed successfully!" << endl;
                            break;
                        case 3:
                            //update inspection interval code
                            cout << "The extinguisher interval has been changed successfully!" << endl;
                            break;
                        case 4:
                            //add report code
                            cout << "The extinguisher report has been added successfully!" << endl;
                            break;
                        default:
                            cout << "Invalid option. Please try again." << endl;
                            changeDB(access);
                    }
                    break;
                default:
                    cout << "Invalid option. Please try again." << endl;
                    changeDB(access);
            }
        }

        void viewDB(int access){
            accessDB(access);
            //view database code
            cout << "Here is the database." << endl;
        }

        void generateAssignment(){
            //use assignment class?
            //should have # of extinguishers, estimate of # of investigators, or just a list of inspectors/the company, locations of extinguishers, date it must be done by, other info as needed
            //send to investigators or third party company 
            cout << "The report has be generated successfully!" << endl;
        }
}
