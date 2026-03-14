// Inv.cpp
#include "Inv.h"

bool Inv::generateReport(
    sqlite3* db,
    const std::string& extinguisherId,
    float gaugePsi,
    bool obstruction,
    const std::vector<PhotoInput>& photos,
    const std::string& notes,
    std::string* outReportId,
    const std::optional<std::string>& assignmentId
) const {
    if (!db) return false;

    // Basic validation
    if (gaugePsi < 0.0f || gaugePsi > 400.0f) return false;
    for (const auto& p : photos) {
        if (!isAllowedContent(p.contentType)) return false;
        if (p.sizeBytes < 0 || p.sizeBytes > 3*1024*1024) return false; // 3 MB cap example
        if (p.sha256.size() != 64) return false; // hex SHA-256
    }

    // Verify extinguisher belongs to this tenant
    {
        sqlite3_stmt* st=nullptr;
        const char* sql = "SELECT company_id FROM EXTINGUISHER WHERE extinguisher_id=?;";
        if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(st, 1, extinguisherId.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(st);
        if (rc != SQLITE_ROW) { sqlite3_finalize(st); return false; }
        const unsigned char* c = sqlite3_column_text(st, 0);
        std::string exTenant = c ? reinterpret_cast<const char*>(c) : "";
        sqlite3_finalize(st);
        if (exTenant != companyId) return false;
    }

    if (!beginTxn(db)) return false;

    bool ok = false;
    std::string reportId = uuid4();
    std::string createdAt = nowISO();

    sqlite3_stmt* insReport=nullptr;
    const char* insReportSQL =
        "INSERT INTO REPORT (report_id, company_id, assignment_id, extinguisher_id, created_by, "
        "created_at, status, submission_mode, gauge_reading_psi, obstruction, media_integrity, "
        "anomaly_flag, reset_interval_applied, notes, signature) "
        "VALUES (?, ?, ?, ?, ?, ?, 'submitted', 'realtime', ?, ?, 'unchecked', 'none', 0, ?, NULL);";

    if (sqlite3_prepare_v2(db, insReportSQL, -1, &insReport, nullptr) != SQLITE_OK) { rollbackTxn(db); return false; }

    int b = 1;
    sqlite3_bind_text(insReport, b++, reportId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insReport, b++, companyId.c_str(), -1, SQLITE_TRANSIENT);
    if (assignmentId) sqlite3_bind_text(insReport, b++, assignmentId->c_str(), -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(insReport, b++);
    sqlite3_bind_text(insReport, b++, extinguisherId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insReport, b++, userId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insReport, b++, createdAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(insReport, b++, static_cast<double>(gaugePsi));
    sqlite3_bind_int(insReport, b++, obstruction ? 1 : 0);
    sqlite3_bind_text(insReport, b++, notes.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(insReport) != SQLITE_DONE) { sqlite3_finalize(insReport); rollbackTxn(db); return false; }
    sqlite3_finalize(insReport);

    // Insert photos
    const char* insPhotoSQL =
        "INSERT INTO PHOTO (media_id, report_id, content_type, size_bytes, sha256, width, height, captured_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    for (const auto& p : photos) {
        sqlite3_stmt* ps=nullptr;
        if (sqlite3_prepare_v2(db, insPhotoSQL, -1, &ps, nullptr) != SQLITE_OK) { rollbackTxn(db); return false; }
        int qb=1;
        std::string mediaId = uuid4();
        sqlite3_bind_text(ps, qb++, mediaId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ps, qb++, reportId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ps, qb++, p.contentType.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (ps, qb++, p.sizeBytes);
        sqlite3_bind_text(ps, qb++, p.sha256.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (ps, qb++, p.width);
        sqlite3_bind_int (ps, qb++, p.height);
        sqlite3_bind_text(ps, qb++, p.capturedAtISO.empty() ? createdAt.c_str() : p.capturedAtISO.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(ps) != SQLITE_DONE) { sqlite3_finalize(ps); rollbackTxn(db); return false; }
        sqlite3_finalize(ps);
    }

    // Audit
    const char* auditSQL =
        "INSERT INTO AUDIT_LOG (audit_id, company_id, entity_type, entity_id, actor_user_id, action, payload_hash, occurred_at) "
        "VALUES (?, ?, 'Report', ?, ?, 'create', '', ?);";
    {
        sqlite3_stmt* as=nullptr;
        if (sqlite3_prepare_v2(db, auditSQL, -1, &as, nullptr) != SQLITE_OK) { rollbackTxn(db); return false; }
        std::string auditId=uuid4();
        int ab=1;
        sqlite3_bind_text(as, ab++, auditId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(as, ab++, companyId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(as, ab++, reportId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(as, ab++, userId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(as, ab++, createdAt.c_str(), -1, SQLITE_TRANSIENT);
        ok = sqlite3_step(as) == SQLITE_DONE;
        sqlite3_finalize(as);
        if (!ok) { rollbackTxn(db); return false; }
    }

    ok = commitTxn(db);
    if (ok && outReportId) *outReportId = reportId;
    return ok;
}
