#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class SwaggerController : public drogon::HttpController<SwaggerController>
{
public:
    METHOD_LIST_BEGIN
    // Swagger UI page
    ADD_METHOD_TO(SwaggerController::ui,   "/swagger",   Get);
    // OpenAPI JSON spec
    ADD_METHOD_TO(SwaggerController::spec, "/api-docs",  Get);
    METHOD_LIST_END

    void ui(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback);

    void spec(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);
};
