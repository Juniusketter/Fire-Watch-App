#include "AdminRoutes.h"
void registerAdminRoutes(crow::SimpleApp& app){
 CROW_ROUTE(app,"/admin/activate/<int>").methods("POST"_method)
 ([](int){ return crow::response("ACTIVATED"); });
}
