#include "JwtFilter.h"
#include "utils/JwtUtils.h"
#include <jwt/jwt.hpp>
#include <drogon/drogon.h>

void JwtFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    // ── 1. Extract the token ─────────────────────────────────────────────────
    const std::string authHeader = req->getHeader("Authorization");

    if (authHeader.empty() || authHeader.rfind("Bearer ", 0) != 0)
    {
        Json::Value j;
        j["error"] = "Missing or malformed Authorization header";
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(k401Unauthorized);
        fcb(resp);   // stop the chain
        return;
    }

    const std::string token = authHeader.substr(7); // strip "Bearer "

    // ── 2. Verify the token ──────────────────────────────────────────────────
    try
    {
        jwt::jwt_object obj = jwt::decode(
            token,
            jwt::params::algorithms({"HS256"}),
            jwt::params::secret(jwt_utils::secret())
        );

        // ── 3. Forward verified claims as request attributes ─────────────
        auto payload = obj.payload();
        auto attrs = req->getAttributes();
        attrs->insert("jwt_user_id", payload.get_claim_value<std::string>("sub"));
        attrs->insert("jwt_role",    payload.get_claim_value<std::string>("role"));

        fccb();   // pass to next filter / controller
    }
    catch (const jwt::TokenExpiredError &)
    {
        Json::Value j;
        j["error"] = "Token has expired";
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(k401Unauthorized);
        fcb(resp);
    }
    catch (const jwt::SignatureFormatError &)
    {
        Json::Value j;
        j["error"] = "Invalid token signature";
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(k401Unauthorized);
        fcb(resp);
    }
    catch (const std::exception &e)
    {
        Json::Value j;
        j["error"] = std::string("Token error: ") + e.what();
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(k401Unauthorized);
        fcb(resp);
    }
}
