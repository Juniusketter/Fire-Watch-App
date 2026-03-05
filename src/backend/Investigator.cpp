#include <iostream>
using namespace std;

//started by Lilly Bowen on March 4, 2026
//investigator class extends user class

class Investigator : public User{
    private:
        string investigatorID;


    public:
        string getInvestigatorID() {
            return investigatorID;
        }

        void setInvestigatorID(string id) {
            investigatorID = id;
        }

        void generateReport(){
            //generate report code
            //send report to admin
            cout << "Report generated successfully!" << endl;
        }
}
