#pragma once
#include <string>
namespace Storage {
 bool saveEncrypted(const std::string& path,const std::string& data);
 std::string loadDecrypted(const std::string& path);
 std::string aesEncrypt(const std::string& data);
 std::string aesDecrypt(const std::string& data);
}