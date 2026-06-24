#include "BookController.h"
#include <drogon/orm/DbClient.h>

using namespace drogon;
using namespace drogon::orm;

// ─── Helpers ─────────────────────────────────────────────────────────────────

HttpResponsePtr BookController::errorResp(HttpStatusCode code,
                                           const std::string &msg)
{
    Json::Value j;
    j["error"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    return resp;
}

Json::Value BookController::rowToJson(const Row &row)
{
    Json::Value j;
    if (!row["id"].isNull())               j["id"]               = row["id"].as<int32_t>();
    if (!row["title"].isNull())            j["title"]            = row["title"].as<std::string>();
    if (!row["author"].isNull())           j["author"]           = row["author"].as<std::string>();
    if (!row["isbn"].isNull())             j["isbn"]             = row["isbn"].as<std::string>();
    if (!row["publisher"].isNull())        j["publisher"]        = row["publisher"].as<std::string>();
    if (!row["published_year"].isNull())   j["published_year"]   = row["published_year"].as<int32_t>();
    if (!row["genre"].isNull())            j["genre"]            = row["genre"].as<std::string>();
    if (!row["total_copies"].isNull())     j["total_copies"]     = row["total_copies"].as<int32_t>();
    if (!row["available_copies"].isNull()) j["available_copies"] = row["available_copies"].as<int32_t>();
    if (!row["created_at"].isNull())       j["created_at"]       = row["created_at"].as<std::string>();
    if (!row["updated_at"].isNull())       j["updated_at"]       = row["updated_at"].as<std::string>();
    return j;
}

// ─── GET /books ───────────────────────────────────────────────────────────────

void BookController::getAll(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT * FROM books ORDER BY id",
        [callback](const Result &r) {
            Json::Value arr(Json::arrayValue);
            for (const auto &row : r)
                arr.append(rowToJson(row));
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(k200OK);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            callback(errorResp(k500InternalServerError, e.base().what()));
        });
}

// ─── GET /books/{id} ─────────────────────────────────────────────────────────

void BookController::getOne(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             int id)
{
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT * FROM books WHERE id = $1",
        [callback](const Result &r) {
            if (r.empty()) {
                callback(errorResp(k404NotFound, "Book not found"));
                return;
            }
            auto resp = HttpResponse::newHttpJsonResponse(rowToJson(r[0]));
            resp->setStatusCode(k200OK);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            callback(errorResp(k500InternalServerError, e.base().what()));
        },
        id);
}

// ─── POST /books ──────────────────────────────────────────────────────────────

void BookController::create(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        callback(errorResp(k400BadRequest, "Invalid JSON body"));
        return;
    }

    if (!jsonBody->isMember("title") || !(*jsonBody)["title"].isString() ||
        !jsonBody->isMember("author") || !(*jsonBody)["author"].isString() ||
        !jsonBody->isMember("total_copies") || !(*jsonBody)["total_copies"].isInt())
    {
        callback(errorResp(k400BadRequest,
                           "title, author, and total_copies are required"));
        return;
    }

    std::string title        = (*jsonBody)["title"].asString();
    std::string author       = (*jsonBody)["author"].asString();
    std::string isbn         = jsonBody->isMember("isbn")      ? (*jsonBody)["isbn"].asString()      : "";
    std::string publisher    = jsonBody->isMember("publisher") ? (*jsonBody)["publisher"].asString() : "";
    std::string genre        = jsonBody->isMember("genre")     ? (*jsonBody)["genre"].asString()     : "";
    int total    = (*jsonBody)["total_copies"].asInt();
    int available = jsonBody->isMember("available_copies")
                        ? (*jsonBody)["available_copies"].asInt()
                        : total;
    int year = jsonBody->isMember("published_year") ? (*jsonBody)["published_year"].asInt() : 0;

    auto db = app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO books (title, author, isbn, publisher, published_year, genre, "
        "total_copies, available_copies) "
        "VALUES ($1,$2,$3,$4,$5,$6,$7,$8) RETURNING *",
        [callback](const Result &r) {
            auto resp = HttpResponse::newHttpJsonResponse(rowToJson(r[0]));
            resp->setStatusCode(k201Created);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            callback(errorResp(k500InternalServerError, e.base().what()));
        },
        title, author, isbn, publisher, year, genre, total, available);
}

// ─── PUT /books/{id} ─────────────────────────────────────────────────────────

void BookController::update(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             int id)
{
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        callback(errorResp(k400BadRequest, "Invalid JSON body"));
        return;
    }

    // Build SET clause dynamically from provided fields
    std::vector<std::string> setClauses;
    std::vector<std::string> strArgs;
    std::vector<int>         intArgs;

    // We'll collect all values in a single ordered list for the binder
    // Use a simple approach: build the SQL with positional params
    int paramIdx = 1;
    std::string sql = "UPDATE books SET updated_at = NOW()";

    // We pass all possible values but only include set fields
    std::string title, author, isbn, publisher, genre;
    int year = 0, total = 0, available = 0;
    bool hasTitle=false, hasAuthor=false, hasIsbn=false,
         hasPublisher=false, hasGenre=false, hasYear=false,
         hasTotal=false, hasAvailable=false;

    if (jsonBody->isMember("title") && (*jsonBody)["title"].isString()) {
        title = (*jsonBody)["title"].asString(); hasTitle = true;
        sql += ", title = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("author") && (*jsonBody)["author"].isString()) {
        author = (*jsonBody)["author"].asString(); hasAuthor = true;
        sql += ", author = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("isbn") && (*jsonBody)["isbn"].isString()) {
        isbn = (*jsonBody)["isbn"].asString(); hasIsbn = true;
        sql += ", isbn = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("publisher") && (*jsonBody)["publisher"].isString()) {
        publisher = (*jsonBody)["publisher"].asString(); hasPublisher = true;
        sql += ", publisher = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("published_year") && (*jsonBody)["published_year"].isInt()) {
        year = (*jsonBody)["published_year"].asInt(); hasYear = true;
        sql += ", published_year = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("genre") && (*jsonBody)["genre"].isString()) {
        genre = (*jsonBody)["genre"].asString(); hasGenre = true;
        sql += ", genre = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("total_copies") && (*jsonBody)["total_copies"].isInt()) {
        total = (*jsonBody)["total_copies"].asInt(); hasTotal = true;
        sql += ", total_copies = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("available_copies") && (*jsonBody)["available_copies"].isInt()) {
        available = (*jsonBody)["available_copies"].asInt(); hasAvailable = true;
        sql += ", available_copies = $" + std::to_string(++paramIdx);
    }

    if (paramIdx == 1) {
        callback(errorResp(k400BadRequest, "No valid fields to update"));
        return;
    }

    sql += " WHERE id = $1 RETURNING *";

    auto db = app().getDbClient();
    auto binder = *db << sql;
    binder << id;
    if (hasTitle)     binder << title;
    if (hasAuthor)    binder << author;
    if (hasIsbn)      binder << isbn;
    if (hasPublisher) binder << publisher;
    if (hasYear)      binder << year;
    if (hasGenre)     binder << genre;
    if (hasTotal)     binder << total;
    if (hasAvailable) binder << available;

    binder >> [callback](const Result &r) {
        if (r.empty()) {
            callback(errorResp(k404NotFound, "Book not found"));
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(rowToJson(r[0]));
        resp->setStatusCode(k200OK);
        callback(resp);
    };
    binder >> [callback](const DrogonDbException &e) {
        callback(errorResp(k500InternalServerError, e.base().what()));
    };
    binder.exec();
}

// ─── DELETE /books/{id} ──────────────────────────────────────────────────────

void BookController::remove(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback,
                             int id)
{
    auto db = app().getDbClient();
    db->execSqlAsync(
        "DELETE FROM books WHERE id = $1",
        [callback](const Result &r) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            callback(errorResp(k500InternalServerError, e.base().what()));
        },
        id);
}
