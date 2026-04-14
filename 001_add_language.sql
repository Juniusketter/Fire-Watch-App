
ALTER TABLE users
ADD COLUMN language VARCHAR(5) NOT NULL DEFAULT 'en',
ADD COLUMN language_locked BOOLEAN NOT NULL DEFAULT false;

CREATE INDEX idx_users_language ON users(language);
