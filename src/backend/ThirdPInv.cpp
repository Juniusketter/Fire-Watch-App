#include <iostream>
using namespace std;

//started by Lilly Bowen on March 4, 2026
//third party investigator class extends user class

//added by Lilly on March 7
//requirements to be tested throughout process:
//Third party inv object is created
//functional getters & setters
//ensure generate report function works

class ThirdPInv : public User{
    private:
        string thirdPInvID;

    public:
        string getThirdPInvID() {
            return thirdPInvID;
        }
        void setThirdPInvID(string id) {
            thirdPInvID = id;
        }
        void generateThirdPReport(){
            //generate report code
            //send report to admin
            cout << "Report generated successfully!" << endl;
        }
}
