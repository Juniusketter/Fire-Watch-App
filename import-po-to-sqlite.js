import gettextParser from 'gettext-parser';
import fs from 'fs';
import sqlite from 'better-sqlite3';

const db = sqlite('firewatch.db');
const po = gettextParser.po.parse(fs.readFileSync('i18n/es.po'));

const stmt = db.prepare(
  'INSERT OR REPLACE INTO translations (translation_key, language_code, value) VALUES (?, ?, ?)'
);

Object.values(po.translations['']).forEach(entry => {
  if (entry.msgstr && entry.msgstr[0]) {
    stmt.run(entry.msgid, 'es', entry.msgstr[0]);
  }
});