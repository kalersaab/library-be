#pragma once

#include <json/json.h>
#include <string>
#include <memory>
#include <stdint.h>

// Lightweight User DTO.
// Run `drogon_ctl create model models` against a live DB to replace
// this with a fully Mapper-compatible generated model.

namespace drogon_model {
namespace library {

struct User {
    std::shared_ptr<int32_t>     id;
    std::shared_ptr<std::string> name;
    std::shared_ptr<std::string> email;
    std::shared_ptr<std::string> passwordHash; // never exposed in toJson()
    std::shared_ptr<std::string> role;         // "admin" | "member"

    static User fromJson(const Json::Value &j);

    // Serializes all fields except passwordHash
    Json::Value toJson() const;
};

} // namespace library
} // namespace drogon_model
