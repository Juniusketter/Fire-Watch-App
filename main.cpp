
#define CROW_MAIN
#include "crow_all.h"
#include "services/AuthRoutes.h"
int main(){
    crow::SimpleApp app;
    registerAuthRoutes(app);
    app.port(8080).multithreaded().run();
}
