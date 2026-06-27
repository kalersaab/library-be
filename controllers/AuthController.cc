#include "AuthController.h"
#include "utils/JwtUtils.h"
#include <drogon/orm/DbClient.h>
#include <drogon/utils/Utilities.h>
#include <jwt/jwt.hpp>
#include <chrono>

using namespace drogon;
using namespace drogon::orm;

HttpResponsePtr AuthController::errorResp(HttpStatusCode code,
                                           const std::string &msg)
{
    Json::Value j;
    j["error"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    return resp;
}

// POST /auth/login
// Body: { "email": "...", "password": "..." }
void AuthController::login(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto body = req->getJsonObject();
    if (!body)
    {
        callback(errorResp(k400BadRequest, "Invalid JSON body"));
        return;
    }

    if (!body->isMember("email")    || !(*body)["email"].isString() ||
        !body->isMember("password") || !(*body)["password"].isString())
    {
        callback(errorResp(k400BadRequest, "email and password are required"));
        return;
    }

    const std::string email = (*body)["email"].asString();
    const std::string hash  = drogon::utils::getSha256((*body)["password"].asString());

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT id, name, email, role FROM users "
        "WHERE email = $1 AND password_hash = $2 LIMIT 1",
        [callback](const Result &r)
        {
            if (r.empty())
            {
                Json::Value j;
                j["error"] = "Invalid email or password";
                auto resp = HttpResponse::newHttpJsonResponse(j);
                resp->setStatusCode(k401Unauthorized);
                callback(resp);
                return;
            }

            const auto &row = r[0];
            const std::string userId = row["id"].as<std::string>();
            const std::string role   = row["role"].as<std::string>();
            const std::string name   = row["name"].as<std::string>();

            // ── Build the JWT ─────────────────────────────────────────────
            auto now    = std::chrono::system_clock::now();
            auto expiry = now + std::chrono::hours(jwt_utils::expiry_hours);

            jwt::jwt_object token{
                jwt::params::algorithm("HS256"),
                jwt::params::secret(jwt_utils::secret()),
                jwt::params::payload({
                    {"sub",  userId},
                    {"role", role},
                    {"name", name}
                })
            };
            token.add_claim(jwt::registered_claims::expiration, expiry);
            token.add_claim(jwt::registered_claims::issued_at,  now);

            Json::Value resp;
            resp["token"]      = token.signature();
            resp["expires_in"] = jwt_utils::expiry_hours * 3600;
            resp["user"]["id"]    = row["id"].as<std::string>();
            resp["user"]["name"]  = name;
            resp["user"]["email"] = row["email"].as<std::string>();
            resp["user"]["role"]  = role;

            auto httpResp = HttpResponse::newHttpJsonResponse(resp);
            httpResp->setStatusCode(k200OK);
            callback(httpResp);
        },
        [callback](const DrogonDbException &e)
        {
            Json::Value j;
            j["error"] = e.base().what();
            auto resp = HttpResponse::newHttpJsonResponse(j);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        email, hash);
}
