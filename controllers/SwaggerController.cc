#include "SwaggerController.h"

using namespace drogon;

// ─── GET /swagger  ────────────────────────────────────────────────────────────
// Serves the Swagger UI HTML (uses unpkg CDN — no local assets needed)

void SwaggerController::ui(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback)
{
    static const std::string html = R"html(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Library API – Swagger UI</title>
  <link rel="stylesheet"
        href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css"/>
</head>
<body>
<div id="swagger-ui"></div>
<script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
<script>
  window.onload = () => {
    SwaggerUIBundle({
      url: "/api-docs",
      dom_id: "#swagger-ui",
      presets: [SwaggerUIBundle.presets.apis, SwaggerUIBundle.SwaggerUIStandalonePreset],
      layout: "BaseLayout",
      deepLinking: true
    });
  };
</script>
</body>
</html>)html";

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(html);
    callback(resp);
}

// ─── GET /api-docs  ───────────────────────────────────────────────────────────
// Serves the OpenAPI 3.0 spec as JSON

void SwaggerController::spec(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback)
{
    static const std::string specJson = R"json({
  "openapi": "3.0.3",
  "info": {
    "title": "Library API",
    "description": "Backend API for the Library management system",
    "version": "1.0.0"
  },
  "servers": [
    { "url": "http://127.0.0.1:3000", "description": "Local dev server" }
  ],
  "tags": [
    { "name": "Auth",    "description": "Authentication"              },
    { "name": "Books",   "description": "Book inventory management"  },
    { "name": "Users",   "description": "User account management"   },
    { "name": "Borrows", "description": "Borrow and return books"    }
  ],
  "paths": {

    "/auth/login": {
      "post": {
        "tags": ["Auth"],
        "summary": "Login and receive a JWT",
        "operationId": "login",
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "$ref": "#/components/schemas/LoginInput" } } }
        },
        "responses": {
          "200": { "description": "JWT token", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/LoginResponse" } } } },
          "401": { "description": "Invalid credentials", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/books": {
      "get": {
        "tags": ["Books"],
        "summary": "List all books",
        "operationId": "getBooks",
        "responses": {
          "200": {
            "description": "Array of books",
            "content": { "application/json": { "schema": { "type": "array", "items": { "$ref": "#/components/schemas/Book" } } } }
          }
        }
      },
      "post": {
        "tags": ["Books"],
        "summary": "Create a book",
        "operationId": "createBook",
        "security": [{ "bearerAuth": [] }],
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "$ref": "#/components/schemas/BookInput" } } }
        },
        "responses": {
          "201": { "description": "Created",        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Book" } } } },
          "400": { "description": "Bad request",    "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/books/{id}": {
      "parameters": [
        { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
      ],
      "get": {
        "tags": ["Books"],
        "summary": "Get a book by ID",
        "operationId": "getBook",
        "responses": {
          "200": { "description": "OK",        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Book" } } } },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      },
      "put": {
        "tags": ["Books"],
        "summary": "Update a book",
        "operationId": "updateBook",
        "security": [{ "bearerAuth": [] }],
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "$ref": "#/components/schemas/BookInput" } } }
        },
        "responses": {
          "200": { "description": "Updated",   "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Book" } } } },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      },
      "delete": {
        "tags": ["Books"],
        "summary": "Delete a book",
        "operationId": "deleteBook",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "204": { "description": "Deleted"    },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/users": {
      "get": {
        "tags": ["Users"],
        "summary": "List all users",
        "operationId": "getUsers",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "200": {
            "description": "Array of users",
            "content": { "application/json": { "schema": { "type": "array", "items": { "$ref": "#/components/schemas/User" } } } }
          }
        }
      },
      "post": {
        "tags": ["Users"],
        "summary": "Create a user",
        "operationId": "createUser",
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "$ref": "#/components/schemas/UserInput" } } }
        },
        "responses": {
          "201": { "description": "Created",       "content": { "application/json": { "schema": { "$ref": "#/components/schemas/User" } } } },
          "400": { "description": "Bad request",   "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "409": { "description": "Email in use",  "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/users/{id}": {
      "parameters": [
        { "name": "id", "in": "path", "required": true, "schema": { "type": "string", "format": "uuid" } }
      ],
      "get": {
        "tags": ["Users"],
        "summary": "Get a user by ID",
        "operationId": "getUser",
        "responses": {
          "200": { "description": "OK",        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/User" } } } },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      },
      "put": {
        "tags": ["Users"],
        "summary": "Update a user",
        "operationId": "updateUser",
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "$ref": "#/components/schemas/UserUpdateInput" } } }
        },
        "responses": {
          "200": { "description": "Updated",   "content": { "application/json": { "schema": { "$ref": "#/components/schemas/User" } } } },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "409": { "description": "Email in use", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      },
      "delete": {
        "tags": ["Users"],
        "summary": "Delete a user",
        "operationId": "deleteUser",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "204": { "description": "Deleted"    },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/borrows": {
      "get": {
        "tags": ["Borrows"],
        "summary": "List borrow records (admin / librarian)",
        "operationId": "getBorrows",
        "security": [{ "bearerAuth": [] }],
        "parameters": [
          { "name": "user_id", "in": "query", "schema": { "type": "string", "format": "uuid" }, "description": "Filter by member UUID" },
          { "name": "book_id", "in": "query", "schema": { "type": "integer" },                  "description": "Filter by book ID"     },
          { "name": "status",  "in": "query", "schema": { "type": "string", "enum": ["borrowed","returned"] }, "description": "Filter by status" }
        ],
        "responses": {
          "200": { "description": "Array of borrow records", "content": { "application/json": { "schema": { "type": "array", "items": { "$ref": "#/components/schemas/Borrow" } } } } },
          "403": { "description": "Forbidden",  "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      },
      "post": {
        "tags": ["Borrows"],
        "summary": "Issue a book to a member (admin / librarian)",
        "operationId": "borrowBook",
        "security": [{ "bearerAuth": [] }],
        "requestBody": {
          "required": true,
          "content": { "application/json": { "schema": { "$ref": "#/components/schemas/BorrowInput" } } }
        },
        "responses": {
          "201": { "description": "Borrow record created", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Borrow" } } } },
          "400": { "description": "Bad request",  "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "403": { "description": "Forbidden",    "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "404": { "description": "User or book not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "409": { "description": "Not enough copies available", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/borrows/{id}": {
      "parameters": [
        { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
      ],
      "get": {
        "tags": ["Borrows"],
        "summary": "Get a single borrow record",
        "operationId": "getBorrow",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "200": { "description": "OK",        "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Borrow" } } } },
          "403": { "description": "Forbidden", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      },
      "delete": {
        "tags": ["Borrows"],
        "summary": "Hard-delete a borrow record (admin only)",
        "operationId": "deleteBorrow",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "204": { "description": "Deleted"    },
          "403": { "description": "Forbidden", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    },

    "/borrows/{id}/return": {
      "parameters": [
        { "name": "id", "in": "path", "required": true, "schema": { "type": "integer" } }
      ],
      "put": {
        "tags": ["Borrows"],
        "summary": "Mark a book as returned (admin / librarian)",
        "operationId": "returnBook",
        "security": [{ "bearerAuth": [] }],
        "responses": {
          "200": { "description": "Returned",  "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Borrow" } } } },
          "403": { "description": "Forbidden", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "404": { "description": "Not found", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } },
          "409": { "description": "Already returned", "content": { "application/json": { "schema": { "$ref": "#/components/schemas/Error" } } } }
        }
      }
    }
  },

  "components": {
    "securitySchemes": {
      "bearerAuth": {
        "type": "http",
        "scheme": "bearer",
        "bearerFormat": "JWT"
      }
    },
    "schemas": {

      "LoginInput": {
        "type": "object",
        "required": ["email", "password"],
        "properties": {
          "email":    { "type": "string", "format": "email" },
          "password": { "type": "string", "format": "password" }
        }
      },

      "LoginResponse": {
        "type": "object",
        "properties": {
          "token":      { "type": "string" },
          "expires_in": { "type": "integer" },
          "user": { "$ref": "#/components/schemas/User" }
        }
      },

      "Book": {
        "type": "object",
        "properties": {
          "id":               { "type": "integer", "readOnly": true },
          "title":            { "type": "string"  },
          "author":           { "type": "string"  },
          "isbn":             { "type": "string"  },
          "publisher":        { "type": "string"  },
          "published_year":   { "type": "integer" },
          "genre":            { "type": "string"  },
          "total_copies":     { "type": "integer" },
          "available_copies": { "type": "integer" },
          "created_at":       { "type": "string", "format": "date-time", "readOnly": true },
          "updated_at":       { "type": "string", "format": "date-time", "readOnly": true }
        }
      },

      "BookInput": {
        "type": "object",
        "required": ["title", "author", "total_copies"],
        "properties": {
          "title":            { "type": "string"  },
          "author":           { "type": "string"  },
          "isbn":             { "type": "string"  },
          "publisher":        { "type": "string"  },
          "published_year":   { "type": "integer" },
          "genre":            { "type": "string"  },
          "total_copies":     { "type": "integer" },
          "available_copies": { "type": "integer" }
        }
      },

      "User": {
        "type": "object",
        "properties": {
          "id":         { "type": "string", "format": "uuid", "readOnly": true },
          "name":       { "type": "string"  },
          "email":      { "type": "string", "format": "email" },
      "role":       { "type": "string", "enum": ["admin", "librarian", "member"] },
          "created_at": { "type": "string", "format": "date-time", "readOnly": true },
          "updated_at": { "type": "string", "format": "date-time", "readOnly": true }
        }
      },

      "UserInput": {
        "type": "object",
        "required": ["name", "email", "password"],
        "properties": {
          "name":     { "type": "string" },
          "email":    { "type": "string", "format": "email" },
          "password": { "type": "string", "format": "password" },
          "role":     { "type": "string", "enum": ["admin", "librarian", "member"], "default": "member" }
        }
      },

      "UserUpdateInput": {
        "type": "object",
        "properties": {
          "name":     { "type": "string" },
          "email":    { "type": "string", "format": "email" },
          "password": { "type": "string", "format": "password" },
          "role":     { "type": "string", "enum": ["admin", "librarian", "member"] }
        }
      },

      "Error": {
        "type": "object",
        "properties": {
          "error": { "type": "string" }
        }
      },

      "Borrow": {
        "type": "object",
        "properties": {
          "id":             { "type": "integer", "readOnly": true },
          "user_id":        { "type": "string", "format": "uuid" },
          "book_id":        { "type": "integer" },
          "issued_by":      { "type": "string", "format": "uuid", "description": "UUID of the librarian who issued the book" },
          "quantity":       { "type": "integer", "default": 1 },
          "issue_date":     { "type": "string", "format": "date-time", "readOnly": true },
          "due_date":       { "type": "string", "format": "date-time" },
          "return_date":    { "type": "string", "format": "date-time", "readOnly": true },
          "status":         { "type": "string", "enum": ["borrowed", "returned"], "readOnly": true },
          "member_name":    { "type": "string", "readOnly": true },
          "book_title":     { "type": "string", "readOnly": true },
          "librarian_name": { "type": "string", "readOnly": true },
          "created_at":     { "type": "string", "format": "date-time", "readOnly": true },
          "updated_at":     { "type": "string", "format": "date-time", "readOnly": true }
        }
      },

      "BorrowInput": {
        "type": "object",
        "required": ["user_id"],
        "properties": {
          "user_id":    { "type": "string", "format": "uuid", "description": "Member UUID receiving the book" },
          "book_id":    { "type": "integer", "description": "Book ID (use this or book_title)" },
          "book_title": { "type": "string",  "description": "Book title – case-insensitive lookup (use this or book_id)" },
          "quantity":   { "type": "integer", "default": 1 },
          "due_date":   { "type": "string",  "format": "date", "description": "Optional return due date (ISO 8601)" }
        }
      }
    }
  }
})json";

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(specJson);
    // Allow Swagger UI to fetch from a different origin if needed
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}
