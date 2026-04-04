
#include "AuthRoutes.h"
#include <iostream>
void registerAuthRoutes(crow::SimpleApp& app){
  CROW_ROUTE(app, "/health")( [](){ return "OK"; } );
}
