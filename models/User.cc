#include "User.h"

using namespace drogon_model::library;

User User::fromJson(const Json::Value &j)
{
    User u;
    if (j.isMember("name")  && j["name"].isString())
        u.name  = std::make_shared<std::string>(j["name"].asString());
    if (j.isMember("email") && j["email"].isString())
        u.email = std::make_shared<std::string>(j["email"].asString());
    if (j.isMember("role")  && j["role"].isString())
        u.role  = std::make_shared<std::string>(j["role"].asString());
    return u;
}

Json::Value User::toJson() const
{
    Json::Value ret;
    if (id)    ret["id"]    = *id;
    if (name)  ret["name"]  = *name;
    if (email) ret["email"] = *email;
    if (role)  ret["role"]  = *role;
    // passwordHash intentionally omitted
    return ret;
}
