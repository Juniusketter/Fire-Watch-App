// Inv.h
#pragma once
#include <string>
#include <vector>
#include <optional>
#include <ctime>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <sqlite3.h>

struct PhotoInput {
    std::string contentType;   // e.g., "image/webp", "image/heif", "image/jpeg", "image/png"
    std::string sha256;        // hex
    int         sizeBytes{0};  // <= 3 MB typical policy
    int         width{0};
    int         height{0};
    std::string capturedAtISO; // ISO-8601 string (UTC)
};

class Inv {
public:
    // Minimal identity (assumed set at login)
    std::string userId;      // UUID
    std::string companyId;   // tenant UUID
    std::string role = "Inspector";

    Inv(std::string userId_, std::string companyId_)
      : userId(std::move(userId_)), companyId(std::move(companyId_)) {}

    // Generate a report for a tenant inspector.
    // Returns true on success; writes new report_id to outReportId if provided.
    bool generateReport(
        sqlite3* db,
        const std::string& extinguisherId,
        float gaugePsi,
        bool obstruction,
        const std::vector<PhotoInput>& photos,
        const std::string& notes,
        std::string* outReportId = nullptr,
        const std::optional<std::string>& assignmentId = std::nullopt
    ) const;

private:
    static bool beginTxn(sqlite3* db) {
        return sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) == SQLITE_OK;
    }
    static bool commitTxn(sqlite3* db) {
        return sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
    }
    static void rollbackTxn(sqlite3* db) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }

    static bool isAllowedContent(const std::string& ct) {
        static const char* k[] = {"image/webp","image/heif","image/heic","image/jpeg","image/png"};
        for (auto* v : k) if (ct == v) return true;
        return false;
    }

    static std::string uuid4() {
        // Simple UUIDv4 generator (not cryptographically secure).
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;
        auto r1 = dis(gen), r2 = dis(gen);
        auto to_hex = uint64_t v {
            std::ostringstream oss; oss<<std::hex<<std::setfill('0');
            oss<<std::setw(16)<<v; return oss.str();
        };
        std::string h = to_hex(r1) + to_hex(r2);
        // format 8-4-4-4-12 and set version/variant bits
        h[12] = '4';
        const char variant[] = {'8','9','a','b'};
        h[16] = variant[(r2>>62)&0x3];
        return h.substr(0,8) + "-" + h.substr(8,4) + "-" + h.substr(12,4) + "-" +
               h.substr(16,4) + "-" + h.substr(20,12);
    }

    static std::string nowISO() {
        std::time_t t = std::time(nullptr);
        std::tm g{}; gmtime_r(&t,&g);
        std::ostringstream oss; oss<<std::put_time(&g, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }
};
