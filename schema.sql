CREATE TABLE translations (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  translation_key TEXT NOT NULL,
  language_code TEXT NOT NULL,
  value TEXT NOT NULL,
  UNIQUE (translation_key, language_code)
);