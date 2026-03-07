#include <iostream>
using namespace std;

//started by Lilly Bowen on March 4, 2026
//investigator class extends user class

//Added by Lilly on March 7
//requirements to be tested throughout process:
//Inv object is created
//functional getters & setters
//ensure generate report function works

class Inv : public User{
    private:
        string invID;


    public:
        string getInvID() {
            return invID;
        }

        void setInvID(string id) {
            invID = id;
        }

        void generateReport(){
            //generate report code
            //send report to admin
            cout << "Report generated successfully!" << endl;
        }
}
