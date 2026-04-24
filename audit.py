
from db import db

def log_language_record(org_id, table, record_id, lang, original):
    db.execute(
        "INSERT INTO language_audit_log (org_id, source_table, record_id, language, original_language) VALUES (?, ?, ?, ?, ?)",
        (org_id, table, record_id, lang, original)
    )
    db.commit()
