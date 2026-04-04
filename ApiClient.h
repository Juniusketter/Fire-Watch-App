#pragma once
#include <string>
#include <map>
class ApiClient {
public:
    static std::string get(const std::string& url,
                           const std::map<std::string,std::string>& headers = {});
    static std::string post(const std::string& url,
                            const std::string& jsonBody,
                            const std::map<std::string,std::string>& headers = {});
    static std::string authenticatedGet(const std::string& endpoint);
    static std::string authenticatedPost(const std::string& endpoint,
                                         const std::string& jsonBody);
private:
    static std::string performRequest(const std::string& url,
                                      const std::string* body,
                                      const std::map<std::string,std::string>& headers);
};