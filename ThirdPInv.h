// ThirdPInv.h
#pragma once
#include "Inv.h"

class ThirdPInv {
public:
    std::string userId;       // UUID
    std::string companyId;    // third-party company UUID
    std::string role = "ThirdPartyInspector";

    ThirdPInv(std::string userId_, std::string companyId_)
      : userId(std::move(userId_)), companyId(std::move(companyId_)) {}

    // Third-party inspector must have a valid ACCESS_GRANT for the tenant asset.
    bool generateThirdPReport(
        sqlite3* db,
        const std::string& tenantCompanyId,           // the tenant who owns the asset
        const std::string& extinguisherId,
        float gaugePsi,
        bool obstruction,
        const std::vector<PhotoInput>& photos,
        const std::string& notes,
        std::string* outReportId = nullptr,
        const std::optional<std::string>& assignmentId = std::nullopt,
        const std::optional<std::string>& accessGrantId = std::nullopt
    ) const;

private:
    static bool hasActiveGrant(sqlite3* db,
                               const std::string& tenantCompanyId,
                               const std::string& thirdPartyCompanyId,
                               const std::string& extinguisherId,
                               const std::optional<std::string>& accessGrantId);
};
