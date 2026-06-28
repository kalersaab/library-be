#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>

using namespace drogon;

// ─── BorrowController ────────────────────────────────────────────────────────
// Manages book borrowing and return transactions.
//
// Role rules (enforced in-handler via jwt_role attribute set by JwtFilter):
//   GET    /borrows              – admin, librarian
//   GET    /borrows/{id}         – admin, librarian, or the owning member
//   POST   /borrows              – admin, librarian  (issue a book)
//   PUT    /borrows/{id}/return  – admin, librarian  (mark returned)
//   DELETE /borrows/{id}         – admin only
//
// JwtFilter is declared on all routes; role checks are done inside the handler
// so that the member self-access case on GET /borrows/{id} can be handled.

class BorrowController : public drogon::HttpController<BorrowController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BorrowController::getAll,    "/borrows",              Get,    "JwtFilter");
    ADD_METHOD_TO(BorrowController::getOne,    "/borrows/{id}",         Get,    "JwtFilter");
    ADD_METHOD_TO(BorrowController::borrow,    "/borrows",              Post,   "JwtFilter");
    ADD_METHOD_TO(BorrowController::returnBook,"/borrows/{id}/return",  Put,    "JwtFilter");
    ADD_METHOD_TO(BorrowController::remove,    "/borrows/{id}",         Delete, "JwtFilter");
    METHOD_LIST_END

    // GET  /borrows          – list all borrow records (admin / librarian)
    void getAll(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    // GET  /borrows/{id}     – single record (admin / librarian / owner)
    void getOne(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int id);

    // POST /borrows          – issue a book to a member
    // Body: { user_id, book_id OR book_title, quantity?, due_date? }
    void borrow(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

    // PUT  /borrows/{id}/return  – mark a loan as returned
    void returnBook(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int id);

    // DELETE /borrows/{id}   – hard-delete a record (admin only)
    void remove(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int id);

private:
    static Json::Value rowToJson(const drogon::orm::Row &row);
    static HttpResponsePtr errorResp(HttpStatusCode code, const std::string &msg);

    // Returns true and calls callback(403) if the caller's role is not in allowedRoles
    static bool denyIfNotRole(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &callback,
                               std::initializer_list<std::string_view> allowedRoles);
};
