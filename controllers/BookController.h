#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class BookController : public drogon::HttpController<BookController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BookController::getAll, "/books",      Get);
    ADD_METHOD_TO(BookController::getOne, "/books/{id}", Get);
    ADD_METHOD_TO(BookController::create, "/books",      Post);
    ADD_METHOD_TO(BookController::update, "/books/{id}", Put);
    ADD_METHOD_TO(BookController::remove, "/books/{id}", Delete);
    METHOD_LIST_END

    void getAll(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    void getOne(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int id);

    void create(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    void update(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int id);

    void remove(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int id);

private:
    // Build a Book from a DB result row
    static Json::Value rowToJson(const drogon::orm::Row &row);

    // Build a standard error response
    static HttpResponsePtr errorResp(HttpStatusCode code, const std::string &msg);
};
