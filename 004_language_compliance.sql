
-- Organization forced language
ALTER TABLE organizations
ADD COLUMN forced_language TEXT CHECK (forced_language IN ('en','es'));

ALTER TABLE users
ADD COLUMN language TEXT DEFAULT 'en';

ALTER TABLE inspections ADD COLUMN language TEXT;
ALTER TABLE inspections ADD COLUMN original_language TEXT;

ALTER TABLE service_requests ADD COLUMN language TEXT;
ALTER TABLE service_requests ADD COLUMN original_language TEXT;

CREATE TABLE IF NOT EXISTS language_audit_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  org_id INTEGER NOT NULL,
  source_table TEXT NOT NULL,
  record_id INTEGER NOT NULL,
  language TEXT NOT NULL,
  original_language TEXT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
