#include "Crypto.h"
#include <argon2.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <vector>
namespace Crypto {
static std::string bytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (auto b : bytes)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}
std::string generateSalt(size_t length) {
    std::random_device rd;
    std::vector<uint8_t> salt(length);
    for (size_t i = 0; i < length; i++) salt[i] = rd() & 0xFF;
    return bytesToHex(salt);
}
std::string hashPassword(const std::string& password, const std::string& saltHex) {
    std::vector<uint8_t> salt(saltHex.length() / 2);
    for (size_t i = 0; i < salt.size(); i++)
        salt[i] = std::stoi(saltHex.substr(i*2, 2), nullptr, 16);
    std::vector<uint8_t> hash(32);
    int result = argon2id_hash_raw(2, 1<<16, 1, password.data(), password.size(), salt.data(), salt.size(), hash.data(), hash.size());
    if (result != ARGON2_OK) throw std::runtime_error("Argon2id hashing failed");
    return saltHex + ":" + bytesToHex(hash);
}
bool verifyPassword(const std::string& password, const std::string& fullHash) {
    auto pos = fullHash.find(':');
    if (pos == std::string::npos) return false;
    std::string salt = fullHash.substr(0, pos);
    std::string expected = fullHash.substr(pos+1);
    std::string computed = hashPassword(password, salt).substr(salt.size()+1);
    return computed == expected;
}
}
