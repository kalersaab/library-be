#pragma once

#include <drogon/HttpFilter.h>
#include <initializer_list>
#include <set>
#include <string>

// ─── RoleFilter ───────────────────────────────────────────────────────────────
// Template filter that restricts access to a fixed set of roles.
// JwtFilter MUST run first (it populates the "jwt_role" request attribute).
//
// Usage in a controller:
//   ADD_METHOD_TO(Ctrl::handler, "/path", Post,
//                "JwtFilter", "AdminOrLibrarianFilter");
//
// Two concrete instantiations are provided at the bottom of this file.

template <const char *... Roles>
class RoleFilter : public drogon::HttpFilter<RoleFilter<Roles...>>
{
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback       &&fcb,
                  drogon::FilterChainCallback  &&fccb) override
    {
        static const std::set<std::string> allowed{Roles...};

        auto attrs = req->getAttributes();
        std::string role;
        try { role = attrs->get<std::string>("jwt_role"); }
        catch (...) { role = ""; }

        if (allowed.count(role)) {
            fccb();
            return;
        }

        Json::Value j;
        j["error"] = "Forbidden: insufficient role";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(drogon::k403Forbidden);
        fcb(resp);
    }
};

// ── Named role strings (linkage-safe constexpr char arrays) ──────────────────
inline constexpr char kRoleAdmin[]     = "admin";
inline constexpr char kRoleLibrarian[] = "librarian";
inline constexpr char kRoleMember[]    = "member";

// ── Concrete filter types ─────────────────────────────────────────────────────

// Allows admin only
using AdminFilter = RoleFilter<kRoleAdmin>;

// Allows admin OR librarian
using AdminOrLibrarianFilter = RoleFilter<kRoleAdmin, kRoleLibrarian>;

// Allows any authenticated user (admin, librarian, or member)
using AnyRoleFilter = RoleFilter<kRoleAdmin, kRoleLibrarian, kRoleMember>;
