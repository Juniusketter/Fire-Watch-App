#include "PasswordPolicy.h"
#include <cctype>

bool isValidPassword(const std::string& password)
{
    if (password.length() < 10) return false;

    bool hasLetter=false, hasDigit=false, hasSymbol=false;
    int digitRun = 0;

    for (char c : password) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            hasLetter = true; digitRun = 0;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            hasDigit = true; digitRun++;
            if (digitRun > 2) return false;
        } else if (std::ispunct(static_cast<unsigned char>(c))) {
            hasSymbol = true; digitRun = 0;
        }
    }
    return hasLetter && hasDigit && hasSymbol;
}

std::string passwordValidationMessage(const std::string& password)
{
    if (password.length() < 10)
        return "Password must be at least 10 characters long.";

    bool hasLetter=false, hasDigit=false, hasSymbol=false;
    int digitRun = 0;

    for (char c : password) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            hasLetter=true; digitRun=0;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            hasDigit=true; digitRun++;
            if (digitRun > 2)
                return "Password may not contain more than 2 consecutive numbers.";
        } else if (std::ispunct(static_cast<unsigned char>(c))) {
            hasSymbol=true; digitRun=0;
        }
    }
    if (!hasLetter) return "Password must include at least one letter.";
    if (!hasDigit) return "Password must include at least one number.";
    if (!hasSymbol) return "Password must include at least one symbol.";
    return "";
}
