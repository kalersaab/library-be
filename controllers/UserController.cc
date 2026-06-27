#include "UserController.h"
#include <drogon/orm/DbClient.h>
#include <drogon/utils/Utilities.h>  // drogon::utils::getSha256

using namespace drogon;
using namespace drogon::orm;

// ─── Helpers ─────────────────────────────────────────────────────────────────

HttpResponsePtr UserController::errorResp(HttpStatusCode code,
                                           const std::string &msg)
{
    Json::Value j;
    j["error"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    return resp;
}

// password_hash is intentionally excluded from all responses
Json::Value UserController::rowToJson(const Row &row)
{
    Json::Value j;
    if (!row["id"].isNull())         j["id"]         = row["id"].as<std::string>();
    if (!row["name"].isNull())       j["name"]       = row["name"].as<std::string>();
    if (!row["email"].isNull())      j["email"]       = row["email"].as<std::string>();
    if (!row["role"].isNull())       j["role"]       = row["role"].as<std::string>();
    if (!row["created_at"].isNull()) j["created_at"] = row["created_at"].as<std::string>();
    if (!row["updated_at"].isNull()) j["updated_at"] = row["updated_at"].as<std::string>();
    return j;
}

// ─── GET /users ───────────────────────────────────────────────────────────────

void UserController::getAll(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT id, name, email, role, created_at, updated_at "
        "FROM users ORDER BY id",
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

// ─── GET /users/{id} ─────────────────────────────────────────────────────────

void UserController::getOne(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback,
                              std::string id)
{
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT id, name, email, role, created_at, updated_at "
        "FROM users WHERE id = $1",
        [callback](const Result &r) {
            if (r.empty()) {
                callback(errorResp(k404NotFound, "User not found"));
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

// ─── POST /users ──────────────────────────────────────────────────────────────

void UserController::create(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        callback(errorResp(k400BadRequest, "Invalid JSON body"));
        return;
    }

    // Required fields
    if (!jsonBody->isMember("name")     || !(*jsonBody)["name"].isString()  ||
        !jsonBody->isMember("email")    || !(*jsonBody)["email"].isString() ||
        !jsonBody->isMember("password") || !(*jsonBody)["password"].isString())
    {
        callback(errorResp(k400BadRequest, "name, email, and password are required"));
        return;
    }

    const std::string name     = (*jsonBody)["name"].asString();
    const std::string email    = (*jsonBody)["email"].asString();
    const std::string password = (*jsonBody)["password"].asString();
    const std::string role     = (jsonBody->isMember("role") && (*jsonBody)["role"].isString())
                                     ? (*jsonBody)["role"].asString()
                                     : "member";

    // Hash the password before storing
    const std::string hash = drogon::utils::getSha256(password);

    auto db = app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO users (name, email, password_hash, role) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING id, name, email, role, created_at, updated_at",
        [callback](const Result &r) {
            auto resp = HttpResponse::newHttpJsonResponse(rowToJson(r[0]));
            resp->setStatusCode(k201Created);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            // Unique-constraint violation on email
            std::string what = e.base().what();
            if (what.find("unique") != std::string::npos ||
                what.find("duplicate") != std::string::npos)
                callback(errorResp(k409Conflict, "Email already in use"));
            else
                callback(errorResp(k500InternalServerError, what));
        },
        name, email, hash, role);
}

// ─── PUT /users/{id} ─────────────────────────────────────────────────────────

void UserController::update(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback,
                              std::string id)
{
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        callback(errorResp(k400BadRequest, "Invalid JSON body"));
        return;
    }

    int         paramIdx = 1;
    std::string sql      = "UPDATE users SET updated_at = NOW()";

    std::string name, email, role, hash;
    bool hasName = false, hasEmail = false, hasRole = false, hasPassword = false;

    if (jsonBody->isMember("name") && (*jsonBody)["name"].isString()) {
        name = (*jsonBody)["name"].asString(); hasName = true;
        sql += ", name = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("email") && (*jsonBody)["email"].isString()) {
        email = (*jsonBody)["email"].asString(); hasEmail = true;
        sql += ", email = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("role") && (*jsonBody)["role"].isString()) {
        role = (*jsonBody)["role"].asString(); hasRole = true;
        sql += ", role = $" + std::to_string(++paramIdx);
    }
    if (jsonBody->isMember("password") && (*jsonBody)["password"].isString()) {
        hash = drogon::utils::getSha256((*jsonBody)["password"].asString());
        hasPassword = true;
        sql += ", password_hash = $" + std::to_string(++paramIdx);
    }

    if (paramIdx == 1) {
        callback(errorResp(k400BadRequest, "No valid fields to update"));
        return;
    }

    sql += " WHERE id = $1 "
           "RETURNING id, name, email, role, created_at, updated_at";

    auto db     = app().getDbClient();
    auto binder = *db << sql;
    binder << id;
    if (hasName)     binder << name;
    if (hasEmail)    binder << email;
    if (hasRole)     binder << role;
    if (hasPassword) binder << hash;

    binder >> [callback](const Result &r) {
        if (r.empty()) {
            callback(errorResp(k404NotFound, "User not found"));
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(rowToJson(r[0]));
        resp->setStatusCode(k200OK);
        callback(resp);
    };
    binder >> [callback](const DrogonDbException &e) {
        std::string what = e.base().what();
        if (what.find("unique") != std::string::npos ||
            what.find("duplicate") != std::string::npos)
            callback(errorResp(k409Conflict, "Email already in use"));
        else
            callback(errorResp(k500InternalServerError, what));
    };
    binder.exec();
}

// ─── DELETE /users/{id} ──────────────────────────────────────────────────────

void UserController::remove(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback,
                              std::string id)
{
    auto db = app().getDbClient();
    db->execSqlAsync(
        "DELETE FROM users WHERE id = $1",
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
