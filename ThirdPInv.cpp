// ThirdPInv.cpp
#include "ThirdPInv.h"

static bool isoNowBetween(sqlite3_stmt* st) { (void)st; return true; } // placeholder if you later compare timestamps

bool ThirdPInv::hasActiveGrant(sqlite3* db,
                               const std::string& tenantCompanyId,
                               const std::string& thirdPartyCompanyId,
                               const std::string& extinguisherId,
                               const std::optional<std::string>& accessGrantId) {
    // Validate ACCESS_GRANT scope for the extinguisher (or its building) and time window
    const char* sql =
        "SELECT g.grant_id "
        "FROM ACCESS_GRANT g "
        "JOIN EXTINGUISHER e ON (e.extinguisher_id=? AND (g.building_id IS NULL OR g.building_id=e.building_id) "
        "                        AND (g.extinguisher_id IS NULL OR g.extinguisher_id=e.extinguisher_id)) "
        "WHERE g.tenant_company_id=? AND g.third_party_company_id=? "
        "  AND g.scope IN ('inspect','read') "
        "  AND datetime('now') BETWEEN g.starts_at AND g.ends_at "
        "  AND (? IS NULL OR g.grant_id = ?)"
        "LIMIT 1;";

    sqlite3_stmt* st=nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return false;

    int b=1;
    sqlite3_bind_text(st, b++, extinguisherId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, b++, tenantCompanyId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, b++, thirdPartyCompanyId.c_str(), -1, SQLITE_TRANSIENT);
    if (accessGrantId) {
        sqlite3_bind_text(st, b++, accessGrantId->c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st, b++, accessGrantId->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(st, b++); sqlite3_bind_null(st, b++);
    }

    int rc = sqlite3_step(st);
    bool ok = (rc == SQLITE_ROW);
    sqlite3_finalize(st);
    return ok;
}

bool ThirdPInv::generateThirdPReport(
    sqlite3* db,
    const std::string& tenantCompanyId,
    const std::string& extinguisherId,
    float gaugePsi,
    bool obstruction,
    const std::vector<PhotoInput>& photos,
    const std::string& notes,
    std::string* outReportId,
    const std::optional<std::string>& assignmentId,
    const std::optional<std::string>& accessGrantId
) const {
    if (!db) return false;

    // First, prove an active grant for this asset
    if (!hasActiveGrant(db, tenantCompanyId, companyId, extinguisherId, accessGrantId)) return false;

    // Delegate to tenant-style creation but force companyId to tenant scope in REPORT
    // Quick way: temporarily construct a tenant-like Inv with tenant company id but THIS user id (third-party)
    Inv shim(userId, tenantCompanyId);
    return shim.generateReport(db, extinguisherId, gaugePsi, obstruction, photos, notes, outReportId, assignmentId);
}
