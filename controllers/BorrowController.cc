#include "BorrowController.h"
#include <drogon/orm/DbClient.h>

using namespace drogon;
using namespace drogon::orm;

// ─── Helpers ─────────────────────────────────────────────────────────────────

HttpResponsePtr BorrowController::errorResp(HttpStatusCode code,
                                             const std::string &msg)
{
    Json::Value j;
    j["error"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    return resp;
}

// Serialise a borrows row (joined with user name, book title, issued_by name)
Json::Value BorrowController::rowToJson(const Row &row)
{
    Json::Value j;
    if (!row["id"].isNull())          j["id"]          = row["id"].as<int32_t>();
    if (!row["user_id"].isNull())     j["user_id"]     = row["user_id"].as<std::string>();
    if (!row["book_id"].isNull())     j["book_id"]     = row["book_id"].as<int32_t>();
    if (!row["issued_by"].isNull())   j["issued_by"]   = row["issued_by"].as<std::string>();
    if (!row["quantity"].isNull())    j["quantity"]    = row["quantity"].as<int32_t>();
    if (!row["issue_date"].isNull())  j["issue_date"]  = row["issue_date"].as<std::string>();
    if (!row["due_date"].isNull())    j["due_date"]    = row["due_date"].as<std::string>();
    if (!row["return_date"].isNull()) j["return_date"] = row["return_date"].as<std::string>();
    if (!row["status"].isNull())      j["status"]      = row["status"].as<std::string>();
    if (!row["created_at"].isNull())  j["created_at"]  = row["created_at"].as<std::string>();
    if (!row["updated_at"].isNull())  j["updated_at"]  = row["updated_at"].as<std::string>();

    // Optional join columns (present when using the enriched SELECT)
    try { if (!row["member_name"].isNull())    j["member_name"]    = row["member_name"].as<std::string>(); } catch (...) {}
    try { if (!row["book_title"].isNull())     j["book_title"]     = row["book_title"].as<std::string>();  } catch (...) {}
    try { if (!row["librarian_name"].isNull()) j["librarian_name"] = row["librarian_name"].as<std::string>(); } catch (...) {}

    return j;
}

// Returns true (and fires a 403) when the caller's role is not in allowedRoles.
bool BorrowController::denyIfNotRole(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &callback,
    std::initializer_list<std::string_view> allowedRoles)
{
    std::string role;
    try { role = req->getAttributes()->get<std::string>("jwt_role"); } catch (...) {}

    for (auto r : allowedRoles)
        if (role == r) return false;   // allowed – continue

    callback(errorResp(k403Forbidden, "Forbidden: insufficient role"));
    return true;
}

// ─── Enriched SELECT used by getAll / getOne ──────────────────────────────────
static const char* kSelectBorrows =
    "SELECT b.*, "
    "       u.name  AS member_name, "
    "       bk.title AS book_title, "
    "       lib.name AS librarian_name "
    "FROM borrows b "
    "JOIN users  u   ON u.id   = b.user_id "
    "JOIN books  bk  ON bk.id  = b.book_id "
    "JOIN users  lib ON lib.id = b.issued_by ";

// ─── GET /borrows ─────────────────────────────────────────────────────────────

void BorrowController::getAll(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback)
{
    if (denyIfNotRole(req, callback, {"admin", "librarian"})) return;

    // Optional filters via query params: ?user_id=&book_id=&status=
    std::string sql = std::string(kSelectBorrows);
    std::vector<std::string> where;

    const std::string userId  = req->getParameter("user_id");
    const std::string bookId  = req->getParameter("book_id");
    const std::string status  = req->getParameter("status");

    // We'll push params in order; use a simple numbered-param accumulator
    int paramIdx = 0;
    std::vector<std::string> strParams;

    if (!userId.empty()) {
        where.push_back("b.user_id = $" + std::to_string(++paramIdx));
        strParams.push_back(userId);
    }
    if (!bookId.empty()) {
        where.push_back("b.book_id = $" + std::to_string(++paramIdx));
        strParams.push_back(bookId);
    }
    if (!status.empty()) {
        where.push_back("b.status = $" + std::to_string(++paramIdx));
        strParams.push_back(status);
    }

    if (!where.empty()) {
        sql += "WHERE ";
        for (size_t i = 0; i < where.size(); ++i) {
            if (i) sql += "AND ";
            sql += where[i] + " ";
        }
    }
    sql += "ORDER BY b.id";

    auto db     = app().getDbClient();
    auto binder = *db << sql;
    for (const auto &p : strParams) binder << p;

    binder >> [callback](const Result &r) {
        Json::Value arr(Json::arrayValue);
        for (const auto &row : r)
            arr.append(rowToJson(row));
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        resp->setStatusCode(k200OK);
        callback(resp);
    };
    binder >> [callback](const DrogonDbException &e) {
        callback(errorResp(k500InternalServerError, e.base().what()));
    };
    binder.exec();
}

// ─── GET /borrows/{id} ───────────────────────────────────────────────────────

void BorrowController::getOne(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback,
                               int id)
{
    // Members can only see their own records; admin/librarian can see all
    std::string callerRole, callerId;
    try {
        callerRole = req->getAttributes()->get<std::string>("jwt_role");
        callerId   = req->getAttributes()->get<std::string>("jwt_user_id");
    } catch (...) {}

    auto db = app().getDbClient();
    std::string sql = std::string(kSelectBorrows) + "WHERE b.id = $1";

    db->execSqlAsync(
        sql,
        [callback, callerRole, callerId](const Result &r) {
            if (r.empty()) {
                callback(errorResp(k404NotFound, "Borrow record not found"));
                return;
            }
            // Members may only view their own records
            if (callerRole == "member") {
                const std::string ownerId = r[0]["user_id"].as<std::string>();
                if (ownerId != callerId) {
                    callback(errorResp(k403Forbidden, "Forbidden: not your record"));
                    return;
                }
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

// ─── POST /borrows ────────────────────────────────────────────────────────────
// Body (JSON):
// {
//   "user_id"    : "<uuid>",          // member receiving the book
//   "book_id"    : 42,                // preferred – use book ID directly
//   "book_title" : "Clean Code",      // alternative – looked up by title (ILIKE)
//   "quantity"   : 1,                 // default 1
//   "due_date"   : "2026-07-31"       // optional ISO-8601 date
// }

void BorrowController::borrow(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback)
{
    if (denyIfNotRole(req, callback, {"admin", "librarian"})) return;

    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        callback(errorResp(k400BadRequest, "Invalid JSON body"));
        return;
    }

    if (!jsonBody->isMember("user_id") || !(*jsonBody)["user_id"].isString()) {
        callback(errorResp(k400BadRequest, "user_id (UUID) is required"));
        return;
    }
    const bool hasBookId    = jsonBody->isMember("book_id")    && (*jsonBody)["book_id"].isInt();
    const bool hasBookTitle = jsonBody->isMember("book_title") && (*jsonBody)["book_title"].isString();

    if (!hasBookId && !hasBookTitle) {
        callback(errorResp(k400BadRequest, "book_id or book_title is required"));
        return;
    }

    const std::string userId    = (*jsonBody)["user_id"].asString();
    const int         quantity  = (jsonBody->isMember("quantity") && (*jsonBody)["quantity"].isInt())
                                      ? (*jsonBody)["quantity"].asInt() : 1;
    const std::string dueDate   = (jsonBody->isMember("due_date") && (*jsonBody)["due_date"].isString())
                                      ? (*jsonBody)["due_date"].asString() : "";

    // Get the issuer id from JWT
    std::string issuedBy;
    try { issuedBy = req->getAttributes()->get<std::string>("jwt_user_id"); } catch (...) {}

    if (issuedBy.empty()) {
        callback(errorResp(k401Unauthorized, "Could not determine issuer from token"));
        return;
    }

    auto db = app().getDbClient();

    // ── Step 1: resolve book_id (may require a lookup by title) ──────────────
    auto doInsert = [=](int bookId) {
        // ── Step 2: check available copies ───────────────────────────────────
        db->execSqlAsync(
            "SELECT available_copies FROM books WHERE id = $1",
            [=](const Result &r) {
                if (r.empty()) {
                    callback(errorResp(k404NotFound, "Book not found"));
                    return;
                }
                const int available = r[0]["available_copies"].as<int32_t>();
                if (available < quantity) {
                    callback(errorResp(k409Conflict,
                        "Not enough copies available (have " +
                        std::to_string(available) + ", requested " +
                        std::to_string(quantity) + ")"));
                    return;
                }

                // ── Step 3: insert borrow record + decrement copies ───────────
                // Use a transaction for atomicity
                auto txn = db->newTransaction();

                // Insert borrow
                std::string insertSql =
                    "INSERT INTO borrows "
                    "(user_id, book_id, issued_by, quantity, issue_date";
                std::string valuesSql =
                    "VALUES ($1, $2, $3, $4, NOW()";

                int pIdx = 4;
                if (!dueDate.empty()) {
                    insertSql += ", due_date";
                    valuesSql += ", $" + std::to_string(++pIdx);
                }
                insertSql += ") ";
                valuesSql += ") ";
                std::string fullInsert = insertSql + valuesSql +
                    "RETURNING id, user_id, book_id, issued_by, quantity, "
                    "issue_date, due_date, return_date, status, created_at, updated_at";

                auto binder = *txn << fullInsert;
                binder << userId << bookId << issuedBy << quantity;
                if (!dueDate.empty()) binder << dueDate;

                binder >> [=](const Result &insertR) {
                    // Decrement available_copies
                    txn->execSqlAsync(
                        "UPDATE books SET available_copies = available_copies - $1, "
                        "updated_at = NOW() WHERE id = $2",
                        [=](const Result &) {
                            // Enrich with join data
                            db->execSqlAsync(
                                std::string(kSelectBorrows) + "WHERE b.id = $1",
                                [callback](const Result &enriched) {
                                    auto resp = HttpResponse::newHttpJsonResponse(
                                        rowToJson(enriched[0]));
                                    resp->setStatusCode(k201Created);
                                    callback(resp);
                                },
                                [callback](const DrogonDbException &e) {
                                    callback(errorResp(k500InternalServerError, e.base().what()));
                                },
                                insertR[0]["id"].as<int32_t>());
                        },
                        [callback](const DrogonDbException &e) {
                            callback(errorResp(k500InternalServerError, e.base().what()));
                        },
                        quantity, bookId);
                };
                binder >> [callback](const DrogonDbException &e) {
                    callback(errorResp(k500InternalServerError, e.base().what()));
                };
                binder.exec();
            },
            [callback](const DrogonDbException &e) {
                callback(errorResp(k500InternalServerError, e.base().what()));
            },
            bookId);
    };

    if (hasBookId) {
        doInsert((*jsonBody)["book_id"].asInt());
    } else {
        // Look up by title (case-insensitive)
        const std::string title = (*jsonBody)["book_title"].asString();
        db->execSqlAsync(
            "SELECT id FROM books WHERE title ILIKE $1 LIMIT 1",
            [callback, doInsert](const Result &r) {
                if (r.empty()) {
                    callback(errorResp(k404NotFound, "Book not found by title"));
                    return;
                }
                doInsert(r[0]["id"].as<int32_t>());
            },
            [callback](const DrogonDbException &e) {
                callback(errorResp(k500InternalServerError, e.base().what()));
            },
            title);
    }
}

// ─── PUT /borrows/{id}/return ─────────────────────────────────────────────────

void BorrowController::returnBook(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback,
                                   int id)
{
    if (denyIfNotRole(req, callback, {"admin", "librarian"})) return;

    auto db = app().getDbClient();

    // Fetch the borrow record first
    db->execSqlAsync(
        "SELECT id, book_id, quantity, status FROM borrows WHERE id = $1",
        [=](const Result &r) {
            if (r.empty()) {
                callback(errorResp(k404NotFound, "Borrow record not found"));
                return;
            }
            const std::string status = r[0]["status"].as<std::string>();
            if (status == "returned") {
                callback(errorResp(k409Conflict, "Book already returned"));
                return;
            }

            const int bookId   = r[0]["book_id"].as<int32_t>();
            const int quantity = r[0]["quantity"].as<int32_t>();

            // Mark returned and restore copies atomically
            auto txn = db->newTransaction();

            txn->execSqlAsync(
                "UPDATE borrows "
                "SET status = 'returned', return_date = NOW(), updated_at = NOW() "
                "WHERE id = $1 "
                "RETURNING id, user_id, book_id, issued_by, quantity, "
                "          issue_date, due_date, return_date, status, created_at, updated_at",
                [=](const Result &updated) {
                    txn->execSqlAsync(
                        "UPDATE books SET available_copies = available_copies + $1, "
                        "updated_at = NOW() WHERE id = $2",
                        [=](const Result &) {
                            // Enrich with join data
                            db->execSqlAsync(
                                std::string(kSelectBorrows) + "WHERE b.id = $1",
                                [callback](const Result &enriched) {
                                    auto resp = HttpResponse::newHttpJsonResponse(
                                        rowToJson(enriched[0]));
                                    resp->setStatusCode(k200OK);
                                    callback(resp);
                                },
                                [callback](const DrogonDbException &e) {
                                    callback(errorResp(k500InternalServerError, e.base().what()));
                                },
                                updated[0]["id"].as<int32_t>());
                        },
                        [callback](const DrogonDbException &e) {
                            callback(errorResp(k500InternalServerError, e.base().what()));
                        },
                        quantity, bookId);
                },
                [callback](const DrogonDbException &e) {
                    callback(errorResp(k500InternalServerError, e.base().what()));
                },
                id);
        },
        [callback](const DrogonDbException &e) {
            callback(errorResp(k500InternalServerError, e.base().what()));
        },
        id);
}

// ─── DELETE /borrows/{id} ─────────────────────────────────────────────────────

void BorrowController::remove(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback,
                               int id)
{
    if (denyIfNotRole(req, callback, {"admin"})) return;

    auto db = app().getDbClient();
    db->execSqlAsync(
        "DELETE FROM borrows WHERE id = $1",
        [callback](const Result &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            callback(errorResp(k500InternalServerError, e.base().what()));
        },
        id);
}
