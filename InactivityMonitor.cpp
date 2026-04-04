#include "InactivityMonitor.h"
#include "SessionManager.h"
#include <thread>
#include <chrono>
#include <iostream>
using namespace std::chrono;
void startInactivityMonitor(){std::thread([](){while(true){std::this_thread::sleep_for(seconds(10));if(!SessionManager::isLoggedIn())continue;if(SessionManager::isInactivityExpired()||SessionManager::isTokenExpired()){SessionManager::logout();continue;}SessionManager::renewTokenIfAllowed();}}).detach();}