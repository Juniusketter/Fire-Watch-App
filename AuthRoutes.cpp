#include "AuthRoutes.h"
#include "../services/JWTService.h"
#include "../services/TwoFactorService.h"
void registerAuthRoutes(crow::SimpleApp& app){
 CROW_ROUTE(app,"/auth/login").methods("POST"_method)
 ([](){ return crow::response("2FA_REQUIRED"); });
 CROW_ROUTE(app,"/auth/2fa/verify").methods("POST"_method)
 ([](){ return crow::response("JWT_TOKEN"); });
 CROW_ROUTE(app,"/auth/password-reset/request").methods("POST"_method)
 ([](){ return crow::response("RESET_SENT"); });
 CROW_ROUTE(app,"/auth/password-reset/confirm").methods("POST"_method)
 ([](){ return crow::response("PASSWORD_UPDATED"); });
}
