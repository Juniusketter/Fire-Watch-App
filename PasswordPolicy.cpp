#include "PasswordPolicy.h"
#include <cctype>

int passwordStrengthScore(const std::string& password)
{
    int score = 0;
    bool hasLetter=false, hasDigit=false, hasSymbol=false;
    int digitRun = 0;

    if (password.length() >= 10) score += 2;

    for (char c : password) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            hasLetter=true; digitRun=0;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            hasDigit=true; digitRun++;
            if (digitRun > 2) return 0;
        } else if (std::ispunct(static_cast<unsigned char>(c))) {
            hasSymbol=true; digitRun=0;
        }
    }

    if (hasLetter) score++;
    if (hasDigit) score++;
    if (hasSymbol) score++;

    return score; // 0–5
}
