-- ============================================================
-- Library Management System – initial schema
-- Run once against the 'library' PostgreSQL database
-- ============================================================

-- ── Users ────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS users (
    id            UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    name          TEXT        NOT NULL,
    email         TEXT        UNIQUE NOT NULL,
    password_hash TEXT        NOT NULL,
    role          TEXT        NOT NULL DEFAULT 'member'
                              CHECK (role IN ('admin', 'librarian', 'member')),
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Books ────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS books (
    id               SERIAL      PRIMARY KEY,
    title            TEXT        NOT NULL,
    author           TEXT        NOT NULL,
    isbn             TEXT,
    publisher        TEXT,
    published_year   INTEGER,
    genre            TEXT,
    total_copies     INTEGER     NOT NULL DEFAULT 1 CHECK (total_copies >= 0),
    available_copies INTEGER     NOT NULL DEFAULT 1 CHECK (available_copies >= 0),
    created_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at       TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Borrows ──────────────────────────────────────────────────
-- One row per borrow transaction (a member borrowing N copies of a book)
CREATE TABLE IF NOT EXISTS borrows (
    id           SERIAL      PRIMARY KEY,
    user_id      UUID        NOT NULL REFERENCES users(id)  ON DELETE CASCADE,
    book_id      INTEGER     NOT NULL REFERENCES books(id)  ON DELETE CASCADE,
    issued_by    UUID        NOT NULL REFERENCES users(id),   -- librarian who issued
    quantity     INTEGER     NOT NULL DEFAULT 1 CHECK (quantity > 0),
    issue_date   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    due_date     TIMESTAMPTZ,                                  -- optional due date
    return_date  TIMESTAMPTZ,                                  -- NULL = not yet returned
    status       TEXT        NOT NULL DEFAULT 'borrowed'
                             CHECK (status IN ('borrowed', 'returned')),
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
