#define CROW_MAIN
#include "crow_all.h"
#include "routes/AuthRoutes.h"
#include "routes/AdminRoutes.h"
int main(){
    crow::SimpleApp app;
    registerAuthRoutes(app);
    registerAdminRoutes(app);
    app.port(8080).multithreaded().run();
}
