#include <iostream>
using namespace std;

//started by Lilly Bowen on March 3, 2026
//creates user class that will be extended by Admin, Investigator, ThirdPartyAdmin, & ThirdPartyInvestigator
//some of the text in this class & in others will be primarily for testing purposes & can/will be taken out later

//added by Lilly on March 6
//requirements to be tested throughout process:
//User object is created
//functional getters & setters
//login and logout functions work, and will direct you to the appropriate page

class User{
    private:
        string username;
        string password;
        string companyName;

    public:

        //maybe add parametized constructor to cut down on code length

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
        //will need to be connected to the build-auth-zip.yml file, but that might need to be done through the main.cpp file that has not been created yet. 
        void login() {
            string u;
            string p;
            cout << "Username: ";
            cin >> u;
            cout << "Password: ";
            cin >> p;
            if(u != username || p != password){
                cout << "Invalid username or password. Please try again." << endl;
                login();
            }
            else{
                cout << "Login successful!" << endl;
                // Redirect to appropriate dashboard based on user type
            }
        }

        void logout() {
            cout << "Logout successful!" << endl;
            // Redirect to login page
        }
}
