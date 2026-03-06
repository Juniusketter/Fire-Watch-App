#include <iostream>
using namespace std;

//started by Lilly Bowen on March 4, 2026
//third party investigator class extends user class

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
