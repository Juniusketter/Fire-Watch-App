CREATE TABLE inspectors(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 username TEXT UNIQUE,
 password_hash TEXT,
 email TEXT,
 phone TEXT,
 role TEXT,
 status TEXT,
 failed_attempts INTEGER DEFAULT 0,
 lock_until DATETIME
);
CREATE TABLE two_fa_codes(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 inspector_id INTEGER,
 code TEXT,
 expires_at DATETIME
);
CREATE TABLE password_resets(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 inspector_id INTEGER,
 token TEXT,
 expires_at DATETIME
);
CREATE TABLE audit_logs(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 action TEXT,
 actor INTEGER,
 target INTEGER,
 created DATETIME DEFAULT CURRENT_TIMESTAMP
);