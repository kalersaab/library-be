#pragma once

#include <drogon/HttpFilter.h>

using namespace drogon;
extern std::string JWT_SECRET;

// Validates the Authorization: Bearer <token> header on every request
// that uses this filter. On success it forwards the request; on failure
// it returns 401 immediately.
//
// Usage in a controller:
//   ADD_METHOD_TO(Ctrl::handler, "/path", Get, "JwtFilter");

class JwtFilter : public drogon::HttpFilter<JwtFilter>
{
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};
