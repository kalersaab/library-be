#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class UserController : public drogon::HttpController<UserController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::getAll, "/users",      Get,    "JwtFilter");
    ADD_METHOD_TO(UserController::getOne, "/users/{id}", Get,    "JwtFilter");
    ADD_METHOD_TO(UserController::create, "/users",      Post);
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
};
