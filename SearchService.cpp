
#include "SearchService.h"
#include <sstream>
#include <algorithm>
#include <cctype>

static inline std::string toLower(std::string s){ std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }

std::string SearchService::trim(const std::string& s){
    size_t a=s.find_first_not_of(" 	
"); size_t b=s.find_last_not_of(" 	
");
    if(a==std::string::npos) return ""; return s.substr(a, b-a+1);
}

bool SearchService::rebuildExtinguisherFTS(){
    const char* del = "DELETE FROM EXTINGUISHER_FTS;";
    if(sqlite3_exec(db_, del, nullptr, nullptr, nullptr)!=SQLITE_OK) return false;
    const char* ins =
        "INSERT INTO EXTINGUISHER_FTS(extinguisher_id, tenant_id, serial, building, floor, room, location_note, type, status)
"
        "SELECT e.extinguisher_id, e.company_id, e.serial_number, b.name_or_number, e.floor, e.room, e.location_note, e.type, e.status
"
        "FROM EXTINGUISHER e LEFT JOIN BUILDING b ON b.building_id=e.building_id;";
    return sqlite3_exec(db_, ins, nullptr, nullptr, nullptr)==SQLITE_OK;
}

std::string SearchService::buildExtinguisherMatch(const std::string& rawQuery, std::vector<std::string>& terms){
    std::string q = trim(rawQuery);
    if(q.empty()) { terms.clear(); return std::string("*"); }

    std::vector<std::string> tokens; tokens.reserve(8);
    for(size_t i=0;i<q.size();){
        if(q[i]=='"'){
            size_t j=q.find('"', i+1); if(j==std::string::npos) j=q.size()-1;
            std::string phrase=q.substr(i+1, j-i-1); if(!trim(phrase).empty()) tokens.push_back("""+phrase+""");
            i=j+1; continue;
        }
        size_t j=q.find(' ', i); std::string t = (j==std::string::npos)? q.substr(i) : q.substr(i, j-i);
        if(!trim(t).empty()) tokens.push_back(trim(t));
        if(j==std::string::npos) break; i=j+1;
    }
    std::ostringstream match; bool first=true;
    for(auto &t: tokens){
        auto pos=t.find(':');
        if(pos!=std::string::npos && t.front()!='"') continue; // skip field tokens; handled by WHERE
        if(!first) match << " AND "; first=false;
        if(t.size()>=2 && t.front()=='"' && t.back()=='"'){
            match << t; terms.push_back(t.substr(1, t.size()-2));
        } else { match << t << "*"; terms.push_back(t); }
    }
    if(first) match << "*"; // only field filters → match-all
    return match.str();
}

std::vector<ExtinguisherHit> SearchService::searchExtinguishers(
    const std::string& tenantId,
    const std::string& rawQuery,
    const SearchFilters& filters,
    const Page& page,
    int* totalHits
){
    std::vector<ExtinguisherHit> out; out.reserve(page.size);

    std::vector<std::string> terms; std::string match = buildExtinguisherMatch(rawQuery, terms);

    std::ostringstream sql;
    sql << "SELECT extinguisher_id, serial, building, "
        << "snippet(EXTINGUISHER_FTS, 2, '[', ']', '…', 8) AS snip, "
        << "bm25(EXTINGUISHER_FTS, 1.0, 1.2, 1.2, 0.4, 0.4, 0.4, 0.7, 0.6) AS rank "
        << "FROM EXTINGUISHER_FTS WHERE tenant_id=?1 AND EXTINGUISHER_FTS MATCH ?2 ";
    if(filters.type)     sql << "AND type=?3 ";
    if(filters.status)   sql << "AND status=?4 ";
    if(filters.building) sql << "AND building LIKE ?5 ";
    sql << "ORDER BY rank LIMIT ?6 OFFSET ?7;";

    sqlite3_stmt* st=nullptr; if(sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &st, nullptr)!=SQLITE_OK) return out;

    int b=1; sqlite3_bind_text(st, b++, tenantId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, b++, match.c_str(), -1, SQLITE_TRANSIENT);

    if(filters.type)   sqlite3_bind_text(st, b++, filters.type->c_str(), -1, SQLITE_TRANSIENT);
    if(filters.status) sqlite3_bind_text(st, b++, filters.status->c_str(), -1, SQLITE_TRANSIENT);
    if(filters.building){ std::string like = "%"+*filters.building+"%"; sqlite3_bind_text(st, b++, like.c_str(), -1, SQLITE_TRANSIENT);} 

    int size = page.size>0? page.size:20; int offset = (page.page>0? page.page-1:0)*size;
    sqlite3_bind_int(st, b++, size); sqlite3_bind_int(st, b++, offset);

    while(sqlite3_step(st)==SQLITE_ROW){
        ExtinguisherHit h; 
        h.extinguisherId = (const char*)sqlite3_column_text(st,0);
        h.serial         = (const char*)sqlite3_column_text(st,1);
        h.building       = (const char*)sqlite3_column_text(st,2);
        h.snippet        = (const char*)sqlite3_column_text(st,3);
        h.score          = sqlite3_column_double(st,4);
        out.push_back(std::move(h));
    }
    sqlite3_finalize(st);

    if(totalHits){
        const char* csql = "SELECT count(*) FROM EXTINGUISHER_FTS WHERE tenant_id=?1 AND EXTINGUISHER_FTS MATCH ?2;";
        sqlite3_stmt* cs=nullptr; if(sqlite3_prepare_v2(db_, csql, -1, &cs, nullptr)==SQLITE_OK){
            sqlite3_bind_text(cs,1,tenantId.c_str(),-1,SQLITE_TRANSIENT);
            sqlite3_bind_text(cs,2,match.c_str(),-1,SQLITE_TRANSIENT);
            if(sqlite3_step(cs)==SQLITE_ROW) *totalHits = sqlite3_column_int(cs,0);
            sqlite3_finalize(cs);
        }
    }
    return out;
}
