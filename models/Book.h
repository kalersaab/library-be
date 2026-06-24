#pragma once

#include <json/json.h>
#include <string>
#include <memory>
#include <stdint.h>

// Lightweight Book DTO used by BookController.
// If you have a live DB, run:
//   drogon_ctl create model models
// to replace this with a fully Mapper-compatible generated model.

namespace drogon_model {
namespace library {

struct Book {
    std::shared_ptr<int32_t>     id;
    std::shared_ptr<std::string> title;
    std::shared_ptr<std::string> author;
    std::shared_ptr<std::string> isbn;
    std::shared_ptr<std::string> publisher;
    std::shared_ptr<int32_t>     publishedYear;
    std::shared_ptr<std::string> genre;
    std::shared_ptr<int32_t>     totalCopies;
    std::shared_ptr<int32_t>     availableCopies;

    // Populate from a JSON request body
    static Book fromJson(const Json::Value &j);

    // Serialize to JSON response
    Json::Value toJson() const;
};

} // namespace library
} // namespace drogon_model
