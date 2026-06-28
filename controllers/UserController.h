#pragma once

#include <drogon/HttpController.h>
#include <set>
#include <string>

using namespace drogon;

class UserController : public drogon::HttpController<UserController>
{
public:
    METHOD_LIST_BEGIN
    // List / fetch users – admin and librarian only
    ADD_METHOD_TO(UserController::getAll, "/users",      Get,    "JwtFilter");
    ADD_METHOD_TO(UserController::getOne, "/users/{id}", Get,    "JwtFilter");
    // Registration is public (no JWT required)
    ADD_METHOD_TO(UserController::create, "/users",      Post);
    // Update / delete – JWT required; role checks are done inside the handler
    ADD_METHOD_TO(UserController::update, "/users/{id}", Put,    "JwtFilter");
    ADD_METHOD_TO(UserController::remove, "/users/{id}", Delete, "JwtFilter");
    METHOD_LIST_END

    void getAll(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    void getOne(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string id);

    void create(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    void update(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string id);

    void remove(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string id);

private:
    static Json::Value rowToJson(const drogon::orm::Row &row);
    static HttpResponsePtr errorResp(HttpStatusCode code, const std::string &msg);

    // Valid role values accepted by the API
    static const std::set<std::string> kValidRoles;
};
