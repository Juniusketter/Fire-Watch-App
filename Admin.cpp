#include "Admin.h"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <ctime>

static std::string nowISO(){ time_t t=time(nullptr); std::tm g{}; gmtime_r(&t,&g); char b[64]; strftime(b,64,"%Y-%m-%dT%H:%M:%SZ",&g); return b; }

void Admin::audit(sqlite3*db,const std::string&entity,const std::string&id,const std::string&action){
 const char* sql="INSERT INTO AUDIT_LOG(audit_id, company_id, entity_type, entity_id, actor_user_id, action, payload_hash, occurred_at) VALUES(?,?,?,?,?,?,'',?);";
 sqlite3_stmt* st=nullptr; sqlite3_prepare_v2(db, sql, -1, &st, nullptr);
 std::string auditId = "AUD-"+id;
 sqlite3_bind_text(st,1,auditId.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,2,companyId.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,3,entity.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,4,id.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,5,userId.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,6,action.c_str(),-1,SQLITE_TRANSIENT);
 std::string ts=nowISO(); sqlite3_bind_text(st,7,ts.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_step(st); sqlite3_finalize(st);
}

bool Admin::editUser(sqlite3*db,const std::string&uid,const std::string&email,const std::string&name,const std::string&role,std::string*err){
 const char* sql="UPDATE USER SET email=?, display_name=?, role=?, updated_at=? WHERE user_id=? AND company_id=?;";
 sqlite3_stmt* st=nullptr;
 if(sqlite3_prepare_v2(db, sql, -1, &st, nullptr)!=SQLITE_OK){ if(err) *err="prepare failed"; return false; }
 std::string ts=nowISO();
 sqlite3_bind_text(st,1,email.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,2,name.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,3,role.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,4,ts.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,5,uid.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,6,companyId.c_str(),-1,SQLITE_TRANSIENT);
 bool ok=(sqlite3_step(st)==SQLITE_DONE && sqlite3_changes(db)==1);
 sqlite3_finalize(st);
 if(!ok){ if(err) *err="update failed"; return false; }
 audit(db,"User",uid,"update"); return true;
}

bool Admin::deleteUser(sqlite3*db,const std::string&uid,std::string*err){
 const char* sql="DELETE FROM USER WHERE user_id=? AND company_id=?;";
 sqlite3_stmt* st=nullptr;
 sqlite3_prepare_v2(db, sql, -1, &st, nullptr);
 sqlite3_bind_text(st,1,uid.c_str(),-1,SQLITE_TRANSIENT);
 sqlite3_bind_text(st,2,companyId.c_str(),-1,SQLITE_TRANSIENT);
 bool ok=(sqlite3_step(st)==SQLITE_DONE && sqlite3_changes(db)==1);
 sqlite3_finalize(st);
 if(!ok){ if(err) *err="delete failed (FK constraint?)"; return false; }
 audit(db,"User",uid,"delete"); return true;
}