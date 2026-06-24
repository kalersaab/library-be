#include "Book.h"

using namespace drogon_model::library;

Book Book::fromJson(const Json::Value &j)
{
    Book b;
    if (j.isMember("title") && j["title"].isString())
        b.title = std::make_shared<std::string>(j["title"].asString());
    if (j.isMember("author") && j["author"].isString())
        b.author = std::make_shared<std::string>(j["author"].asString());
    if (j.isMember("isbn") && j["isbn"].isString())
        b.isbn = std::make_shared<std::string>(j["isbn"].asString());
    if (j.isMember("publisher") && j["publisher"].isString())
        b.publisher = std::make_shared<std::string>(j["publisher"].asString());
    if (j.isMember("published_year") && j["published_year"].isInt())
        b.publishedYear = std::make_shared<int32_t>(j["published_year"].asInt());
    if (j.isMember("genre") && j["genre"].isString())
        b.genre = std::make_shared<std::string>(j["genre"].asString());
    if (j.isMember("total_copies") && j["total_copies"].isInt())
        b.totalCopies = std::make_shared<int32_t>(j["total_copies"].asInt());
    if (j.isMember("available_copies") && j["available_copies"].isInt())
        b.availableCopies = std::make_shared<int32_t>(j["available_copies"].asInt());
    return b;
}

Json::Value Book::toJson() const
{
    Json::Value ret;
    if (id)             ret["id"]               = *id;
    if (title)          ret["title"]            = *title;
    if (author)         ret["author"]           = *author;
    if (isbn)           ret["isbn"]             = *isbn;
    if (publisher)      ret["publisher"]        = *publisher;
    if (publishedYear)  ret["published_year"]   = *publishedYear;
    if (genre)          ret["genre"]            = *genre;
    if (totalCopies)    ret["total_copies"]     = *totalCopies;
    if (availableCopies)ret["available_copies"] = *availableCopies;
    return ret;
}
