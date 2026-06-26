#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    // POST /auth/login  – returns a signed JWT
    ADD_METHOD_TO(AuthController::login, "/auth/login", Post);
    METHOD_LIST_END

    void login(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

private:
    static HttpResponsePtr errorResp(HttpStatusCode code, const std::string &msg);
};
