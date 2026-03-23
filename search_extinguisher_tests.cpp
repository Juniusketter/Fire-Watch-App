
// tests/search/search_extinguisher_tests.cpp
#include <sqlite3.h>
#include <iostream>
#include <vector>
#include "../../src/search/SearchService.h"

static int exec(sqlite3* db, const char* sql){ char* err=nullptr; int rc=sqlite3_exec(db, sql, nullptr, nullptr, &err); if(rc!=SQLITE_OK){ std::cerr<<"SQL ERR: "<<(err?err:"")<<"
"; sqlite3_free(err);} return rc; }

int main(){
    sqlite3* db=nullptr; if(sqlite3_open(":memory:", &db)!=SQLITE_OK){ std::cerr<<"open fail
"; return 1; }
    // Minimal schema
    const char* ddl =
        "CREATE TABLE COMPANY(company_id TEXT PRIMARY KEY, name TEXT);"
        "CREATE TABLE BUILDING(building_id TEXT PRIMARY KEY, company_id TEXT, name_or_number TEXT);"
        "CREATE TABLE EXTINGUISHER(extinguisher_id TEXT PRIMARY KEY, company_id TEXT, building_id TEXT, serial_number TEXT, type TEXT, status TEXT, floor TEXT, room TEXT, location_note TEXT, updated_at TEXT);";
    if(exec(db, ddl)!=SQLITE_OK) return 1;
    if(exec(db, "INSERT INTO COMPANY VALUES('TENANT-1','TenantCo');")!=SQLITE_OK) return 1;
    if(exec(db, "INSERT INTO BUILDING VALUES('B1','TENANT-1','North Wing');")!=SQLITE_OK) return 1;

    // Seed extinguishers
    exec(db, "INSERT INTO EXTINGUISHER VALUES('X1','TENANT-1','B1','SER-100','ABC','active','2','201','near exit','2026-03-15T00:00:00Z');");
    exec(db, "INSERT INTO EXTINGUISHER VALUES('X2','TENANT-1','B1','SER-200','CO2','active','1','105','lobby','2026-03-15T00:00:00Z');");

    // Create FTS and triggers
    const char* fts =
        "CREATE VIRTUAL TABLE EXTINGUISHER_FTS USING fts5(extinguisher_id UNINDEXED, tenant_id UNINDEXED, serial, building, floor, room, location_note, type, status, content='', tokenize='porter');"
        "CREATE TRIGGER trg_ext_ins AFTER INSERT ON EXTINGUISHER BEGIN
"
        "  INSERT INTO EXTINGUISHER_FTS(extinguisher_id, tenant_id, serial, building, floor, room, location_note, type, status)
"
        "  VALUES (NEW.extinguisher_id, NEW.company_id, NEW.serial_number, (SELECT name_or_number FROM BUILDING WHERE building_id=NEW.building_id), NEW.floor, NEW.room, NEW.location_note, NEW.type, NEW.status); END;";
    if(exec(db, fts)!=SQLITE_OK) return 1;

    // Backfill
    exec(db,
        "INSERT INTO EXTINGUISHER_FTS SELECT e.extinguisher_id, e.company_id, e.serial_number, b.name_or_number, e.floor, e.room, e.location_note, e.type, e.status FROM EXTINGUISHER e LEFT JOIN BUILDING b ON b.building_id=e.building_id;");

    SearchService svc(db);
    SearchFilters filters; filters.type = std::string("ABC");
    Page page{1,10}; int total=0;
    auto hits = svc.searchExtinguishers("TENANT-1", ""near exit"", filters, page, &total);

    std::cout << "total="<<total<<"
";
    for(auto& h : hits){ std::cout << h.extinguisherId << " " << h.serial << " ["<<h.building<<"]
"; }

    sqlite3_close(db);
    return hits.empty(); // return 0 on success (non-empty expected)
}
