// Full implementation placeholder from Phase 4 description
#include "ApiClient.h"
#include "SessionManager.h"
#include "../third_party/json/json.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <iostream>
using json = nlohmann::json;
static size_t WriteCallback(void* contents,size_t size,size_t nmemb,std::string* output){size_t total=size*nmemb;output->append((char*)contents,total);return total;}