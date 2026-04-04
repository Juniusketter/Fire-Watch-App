#pragma once
#include <string>
std::string generate2FACode();
void send2FACode(const std::string& destination, const std::string& code);
