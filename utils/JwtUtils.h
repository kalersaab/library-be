#pragma once

#include <drogon/drogon.h>
#include <string>

namespace jwt_utils {

inline const std::string& secret()
{
    static const std::string s = []() -> std::string {
        const auto &cfg = drogon::app().getCustomConfig();
        if (cfg.isMember("JWT_TOKEN_SECRET") && cfg["JWT_TOKEN_SECRET"].isString())
            return cfg["JWT_TOKEN_SECRET"].asString();

        LOG_WARN << "JWT_TOKEN_SECRET not found in custom_config, using default";
        return std::string("changeme");
    }();
    return s;
}

static constexpr int expiry_hours = 24;

} // namespace jwt_utils
