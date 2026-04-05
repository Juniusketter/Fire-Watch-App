-- Firewatch SQLite Migration: Users Security Schema
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,

    password_hash TEXT NOT NULL,

    security_question_id INTEGER NOT NULL CHECK (
        security_question_id BETWEEN 1 AND 5
    ),
    security_answer_hash TEXT NOT NULL,

    last_security_verified INTEGER NOT NULL,
    failed_security_attempts INTEGER DEFAULT 0,

    created_at INTEGER NOT NULL
);
