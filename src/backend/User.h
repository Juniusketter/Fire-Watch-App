#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
using namespace std;

// Base User class - created by Lilly Bowen
// Extended by Admin, Investigator, ThirdPartyAdmin, ThirdPartyInvestigator
class User {
    private:
        string username;
        string password;
        string companyName;

    public:
        // Default constructor
        User() : username(""), password(""), companyName("") {}

        // Parameterized constructor
        User(string u, string p, string c) : username(u), password(p), companyName(c) {}

        // Virtual destructor for proper polymorphic cleanup
        virtual ~User() {}

        //getters and setters
        void setUsername(string u) {username = u;}
        string getUsername() {return username;}
        void setPassword(string p) {password = p;}
        string getPassword() {return password;}
        void setCompanyName(string c) {companyName = c;}
        string getCompanyName() {return companyName;}

       /* 
            login()
              checks the user's assigned username and password against the input provided 
        
            Parameters:
              u - username input by user
              p - password input by user
        
            Returns: true if usernames and passwords match, false otherwise.
        */ 
        bool login(string u, string p) {
            if (u == username && p == password) {
                cout << "Login successful! Welcome, " << username << "." << endl;
                return true;
            } 
            else {
                cout << "Invalid username or password." << endl;
                return false;
            }
        }

        //logout function
        void logout() {
            cout << "Logout successful! Goodbye, " << username << "." << endl;
        }
};

#endif // USER_H
