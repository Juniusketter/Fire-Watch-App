#include <iostream>
using namespace std;

//started by Lilly Bowen on March 4, 2026
//third party administrator class extends user class

class ThirdPAdmin : public User{
    private:
        string thirdPAdminID;

    public:
        string getThirdPAdminID() {
            return thirdPAdminID;
        }

        void setThirdPAdminID(string id) {
            thirdPAdminID = id;
        }

        void generateThirdPAssignment(){
            //use assignment class?
            //should have # of extinguishers, estimate of # of investigators, or just a list of inspectors/the company, locations of extinguishers, date it must be done by, other info as needed
            //send to investigators or third party company 
        }
}
