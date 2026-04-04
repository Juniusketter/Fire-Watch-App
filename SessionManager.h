#pragma once
#include <string>
#include <chrono>
#include <mutex>

enum class Role { INSPECTOR, ADMIN };
class SessionManager {public:static void login(const std::string&,Role);static void logout();static bool isLoggedIn();static bool isAdmin();static std::string getToken();static bool isTokenExpired();static bool isInactivityExpired();static void updateActivity();static void renewTokenIfAllowed();static std::string getActiveUser();static Role getUserRole();private:static std::string generateToken();static inline bool loggedIn=false;static inline std::string activeUser;static inline Role activeRole;static inline std::string sessionToken;static inline std::chrono::steady_clock::time_point tokenIssuedAt;static inline std::chrono::steady_clock::time_point lastActivityAt;static inline std::mutex sessionMutex;};