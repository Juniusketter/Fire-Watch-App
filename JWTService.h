#pragma once
#include <string>
std::string generateJWT(int userId, const std::string& role);
bool verifyJWT(const std::string& token);
