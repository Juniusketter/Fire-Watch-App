#include <iostream>
using namespace std;

//started by Lilly Bowen on March 3, 2026
//creates user class that will be extended by Admin, Investigator, ThirdPartyAdmin, & ThirdPartyInvestigator
//some of the text in this class & in others will be primarily for testing purposes & can/will be taken out later

class User{
    private:
        string username;
        string password;
        string companyName;

    public:
        void setUsername(string u) {
            username = u;
        }

        string getUsername() {
            return username;
        }

        void setPassword(string p) {
            password = p;
        }

        string getPassword() {
            return password;
        }
        
        //UI could also have "forgot password" or "forgot username" areas that can be added if we choose to
        void login() {}

        void logout() {}
}
