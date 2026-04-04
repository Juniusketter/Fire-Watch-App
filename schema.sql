
CREATE TABLE inspectors (
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 username TEXT UNIQUE,
 password_hash TEXT,
 full_name TEXT,
 organization TEXT,
 phone TEXT,
 email TEXT,
 employee_id TEXT,
 role TEXT,
 status TEXT,
 failed_attempts INTEGER DEFAULT 0,
 lock_until DATETIME,
 created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE audit_logs (
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 actor TEXT,
 action TEXT,
 target TEXT,
 timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);
