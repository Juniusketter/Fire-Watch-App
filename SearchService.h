
#pragma once
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>

struct ExtinguisherHit {
    std::string extinguisherId;
    std::string serial;
    std::string building;
    std::string snippet; // highlighted snippet when FTS is available
    double score{0.0};  // BM25
};

struct SearchFilters {
    std::optional<std::string> type;     // e.g., "ABC", "CO2"
    std::optional<std::string> status;   // e.g., "active"
    std::optional<std::string> building; // free-text like filter
};

struct Page { int page{1}; int size{20}; };

class SearchService {
public:
    explicit SearchService(sqlite3* db) : db_(db) {}

    // Maintenance: rebuild FTS with current base rows
    bool rebuildExtinguisherFTS();

    // Main API: scoped by tenantId
    std::vector<ExtinguisherHit> searchExtinguishers(
        const std::string& tenantId,
        const std::string& rawQuery,
        const SearchFilters& filters,
        const Page& page,
        int* totalHits = nullptr
    );

private:
    sqlite3* db_;
    std::string buildExtinguisherMatch(const std::string& rawQuery, std::vector<std::string>& terms);
    static std::string trim(const std::string& s);
};
