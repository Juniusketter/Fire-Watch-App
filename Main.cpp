// main.cpp
#include <iostream>
#include <functional>
#include <vector>
#include <sqlite3.h>
#include "Inv.h"
#include "ThirdPInv.h"

#define ASSERT_TRUE(expr) do { if(!(expr)) { \
  std::cerr << "[FAIL] " << __FUNCTION__ << ":" << __LINE__ << "  " #expr "\n"; return false; } } while(0)
#define ASSERT_EQ(a,b)   do { if(!((a)==(b))) { \
  std::cerr << "[FAIL] " << __FUNCTION__ << ":" << __LINE__ << "  " #a " == " #b << "  (" << (a) << " vs " << (b) << ")\n"; return false; } } while(0)

static int exec_sql(sqlite3* db, const char* sql) {
    char* err=nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (err?err:"") << "\n";
        sqlite3_free(err);
    }
    return rc;
}

static bool schema(sqlite3* db) {
    const char* ddl =
        "PRAGMA foreign_keys=ON;"
        "CREATE TABLE COMPANY(company_id TEXT PRIMARY KEY, name TEXT, type TEXT, created_at TEXT, updated_at TEXT);"
        "CREATE TABLE USER(user_id TEXT PRIMARY KEY, company_id TEXT, email TEXT UNIQUE, display_name TEXT, role TEXT, password_hash TEXT, created_at TEXT, updated_at TEXT, FOREIGN KEY(company_id) REFERENCES COMPANY(company_id));"
        "CREATE TABLE BUILDING(building_id TEXT PRIMARY KEY, company_id TEXT, address TEXT, name_or_number TEXT, description TEXT, created_at TEXT, updated_at TEXT, FOREIGN KEY(company_id) REFERENCES COMPANY(company_id));"
        "CREATE TABLE EXTINGUISHER(extinguisher_id TEXT PRIMARY KEY, company_id TEXT, building_id TEXT, serial_number TEXT UNIQUE, type TEXT, status TEXT, floor TEXT, room TEXT, location_note TEXT, inspection_interval_days INTEGER, next_due_at TEXT, last_inspection_report_id TEXT, created_at TEXT, updated_at TEXT, FOREIGN KEY(company_id) REFERENCES COMPANY(company_id), FOREIGN KEY(building_id) REFERENCES BUILDING(building_id));"
        "CREATE TABLE ASSIGNMENT(assignment_id TEXT PRIMARY KEY, company_id TEXT, building_id TEXT, inspector_user_id TEXT, created_at TEXT, created_by TEXT, due_by TEXT, status TEXT, priority TEXT, idempotent_key TEXT, notes TEXT);"
        "CREATE TABLE REPORT(report_id TEXT PRIMARY KEY, company_id TEXT, assignment_id TEXT, extinguisher_id TEXT, created_by TEXT, created_at TEXT, status TEXT, submission_mode TEXT, gauge_reading_psi REAL, obstruction INTEGER, media_integrity TEXT, anomaly_flag TEXT, reset_interval_applied INTEGER, notes TEXT, signature TEXT);"
        "CREATE TABLE PHOTO(media_id TEXT PRIMARY KEY, report_id TEXT, content_type TEXT, size_bytes INTEGER, sha256 TEXT, width INTEGER, height INTEGER, captured_at TEXT, FOREIGN KEY(report_id) REFERENCES REPORT(report_id) ON DELETE CASCADE);"
        "CREATE TABLE AUDIT_LOG(audit_id TEXT PRIMARY KEY, company_id TEXT, entity_type TEXT, entity_id TEXT, actor_user_id TEXT, action TEXT, payload_hash TEXT, occurred_at TEXT, FOREIGN KEY(company_id) REFERENCES COMPANY(company_id));"
        "CREATE TABLE ACCESS_GRANT(grant_id TEXT PRIMARY KEY, tenant_company_id TEXT, third_party_company_id TEXT, building_id TEXT, extinguisher_id TEXT, starts_at TEXT, ends_at TEXT, scope TEXT);";
    return exec_sql(db, ddl) == SQLITE_OK;
}

static bool seed(sqlite3* db) {
    return exec_sql(db,
      "INSERT INTO COMPANY VALUES "
      "('TENANT-1111-1111-1111-111111111111','TenantCo','tenant','2026-03-01T00:00:00Z','2026-03-01T00:00:00Z'),"
      "('TP-2222-2222-2222-222222222222','ThirdPartyInc','third_party','2026-03-01T00:00:00Z','2026-03-01T00:00:00Z');"
      "INSERT INTO USER VALUES "
      "('U-TENANT-1','TENANT-1111-1111-1111-111111111111','insp@tenant','Tenant Insp','Inspector','','',''),"
      "('U-TP-1','TP-2222-2222-2222-222222222222','insp@tp','TP Insp','ThirdPartyInspector','','','');"
      "INSERT INTO BUILDING VALUES "
      "('B-AAAA','TENANT-1111-1111-1111-111111111111','100 Safety Way','A','', '', '' );"
      "INSERT INTO EXTINGUISHER VALUES "
      "('X-AAAA','TENANT-1111-1111-1111-111111111111','B-AAAA','SER-123','ABC','active','','','',30,'2026-04-15T00:00:00Z',NULL,'','');"
      "INSERT INTO ACCESS_GRANT VALUES "
      "('G-OK','TENANT-1111-1111-1111-111111111111','TP-2222-2222-2222-222222222222',NULL,'X-AAAA',datetime('now','-1 day'),datetime('now','+1 day'),'inspect');"
    ) == SQLITE_OK;
}

static int count(sqlite3* db, const char* table) {
    std::string q = std::string("SELECT COUNT(*) FROM ") + table + ";";
    sqlite3_stmt* st=nullptr; int n=-1;
    if (sqlite3_prepare_v2(db, q.c_str(), -1, &st, nullptr) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) n = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    return n;
}

static std::vector<PhotoInput> samplePhotos() {
    return {{
        {"image/webp","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",245000,1024,768,""},
        {"image/jpeg","bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",120000, 800,600,""}
    }};
}

// ----- TESTS -----
static bool test1_tenant_report_ok(sqlite3* db) {
    Inv inv("U-TENANT-1","TENANT-1111-1111-1111-111111111111");
    std::string rid;
    bool ok = inv.generateReport(db, "X-AAAA", 175.0f, false, samplePhotos(), "Everything nominal", &rid);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(!rid.empty());
    ASSERT_EQ(count(db,"REPORT"), 1);
    ASSERT_EQ(count(db,"PHOTO"), 2);
    return true;
}

static bool test2_gauge_out_of_range(sqlite3* db) {
    Inv inv("U-TENANT-1","TENANT-1111-1111-1111-111111111111");
    bool ok = inv.generateReport(db, "X-AAAA", 480.0f, false, {}, "bad", nullptr);
    ASSERT_TRUE(!ok);
    ASSERT_EQ(count(db,"REPORT"), 1); // unchanged from previous
    return true;
}

static bool test3_photo_type_reject(sqlite3* db) {
    Inv inv("U-TENANT-1","TENANT-1111-1111-1111-111111111111");
    std::vector<PhotoInput> bad = {{ "image/gif", std::string(64,'c'), 1000, 10,10,""}};
    bool ok = inv.generateReport(db, "X-AAAA", 120.0f, false, bad, "bad", nullptr);
    ASSERT_TRUE(!ok);
    ASSERT_EQ(count(db,"REPORT"), 1);
    return true;
}

static bool test4_thirdparty_ok(sqlite3* db) {
    ThirdPInv tp("U-TP-1","TP-2222-2222-2222-222222222222");
    std::string rid;
    bool ok = tp.generateThirdPReport(db, "TENANT-1111-1111-1111-111111111111", "X-AAAA", 160.0f, false, {}, "tp ok", &rid, std::nullopt, std::make_optional<std::string>("G-OK"));
    ASSERT_TRUE(ok);
    ASSERT_TRUE(!rid.empty());
    ASSERT_EQ(count(db,"REPORT"), 2);
    return true;
}

static bool test5_thirdparty_no_grant(sqlite3* db) {
    ThirdPInv tp("U-TP-1","TP-2222-2222-2222-222222222222");
    bool ok = tp.generateThirdPReport(db, "TENANT-1111-1111-1111-111111111111", "X-AAAA", 160.0f, false, {}, "tp denied", nullptr, std::nullopt, std::make_optional<std::string>("G-NOPE"));
    ASSERT_TRUE(!ok);
    ASSERT_EQ(count(db,"REPORT"), 2);
    return true;
}

static bool test6_audit_rows(sqlite3* db) {
    ASSERT_TRUE(count(db,"AUDIT_LOG") >= 2); // two successful reports created at least 2 audits
    return true;
}

static bool test7_report_links_photos(sqlite3* db) {
    // Check that for the first report we indeed have >= 2 photos
    sqlite3_stmt* st=nullptr;
    ASSERT_TRUE(sqlite3_prepare_v2(db,
        "SELECT r.report_id, (SELECT COUNT(*) FROM PHOTO p WHERE p.report_id=r.report_id) "
        "FROM REPORT r ORDER BY r.created_at LIMIT 1;", -1, &st, nullptr) == SQLITE_OK);
    ASSERT_TRUE(sqlite3_step(st) == SQLITE_ROW);
    int n = sqlite3_column_int(st, 1);
    sqlite3_finalize(st);
    ASSERT_TRUE(n >= 2);
    return true;
}

static bool test8_assignment_optional(sqlite3* db) {
    Inv inv("U-TENANT-1","TENANT-1111-1111-1111-111111111111");
    std::string rid;
    bool ok = inv.generateReport(db, "X-AAAA", 180.0f, true, {}, "with assignment", &rid, std::make_optional<std::string>("A-XYZ"));
    ASSERT_TRUE(ok);
    ASSERT_EQ(count(db,"REPORT"), 3);
    return true;
}

int main() {
    sqlite3* db=nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) { std::cerr<<"DB open failed\n"; return 1; }
    if (!schema(db) || !seed(db)) { std::cerr<<"Failed to init DB\n"; return 1; }

    std::vector<std::pair<std::string,std::function<bool(sqlite3*)>>> tests = {
        {"tenant report ok",           test1_tenant_report_ok},
        {"gauge out of range",         test2_gauge_out_of_range},
        {"photo type reject",          test3_photo_type_reject},
        {"third-party ok",             test4_thirdparty_ok},
        {"third-party denied",         test5_thirdparty_no_grant},
        {"audit rows exist",           test6_audit_rows},
        {"report links photos",        test7_report_links_photos},
        {"assignment optional param",  test8_assignment_optional}
    };

    int fail=0, i=1;
    for (auto& t : tests) {
        bool ok = t.second(db);
        std::cout << (ok? "[PASS] ":"[FAIL] ") << i++ << "/8 " << t.first << "\n";
        if (!ok) fail++;
    }
    sqlite3_close(db);
    std::cout << (fail==0? "\nALL TESTS PASSED ✅\n" : "\nTESTS FAILED ❌ ("+std::to_string(fail)+")\n");
    return fail==0 ? 0 : 1;
}
