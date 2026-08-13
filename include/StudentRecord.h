// ============================================================
//  StudentRecord.h  —  Unified data model
//  Replaces Node1, Node2, and all 30+ separate unordered_maps
//  from the original design.
// ============================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <limits>

using namespace std;

// -------------------------------------------------------
// Round indices — add a new round by extending this enum
// and incrementing NUM_ROUNDS.
// -------------------------------------------------------
enum Round {
    ROUND_1      = 0,
    ROUND_2      = 1,
    ROUND_3      = 2,
    ROUND_4      = 3,
    FINAL_ROUND  = 4,
    NUM_ROUNDS   = 5
};

inline string roundName(int r)
{
    switch (r) {
        case ROUND_1:     return "Round 1";
        case ROUND_2:     return "Round 2";
        case ROUND_3:     return "Round 3";
        case ROUND_4:     return "Round 4";
        case FINAL_ROUND: return "Final Round";
        default:          return "Unknown Round";
    }
}

// -------------------------------------------------------
// Single unified student record — replaces Node1 + Node2.
// `package` is 0.0f for every round except FINAL_ROUND.
// -------------------------------------------------------
struct StudentRecord {
    long long id          = 0;
    string    name;
    int       batch       = 0;
    string    program;
    string    email;
    long long contactNO   = 0;
    long long whatsappNO  = 0;
    string    company;
    int       year        = 0;
    float     package     = 0.0f;   // Only meaningful for FINAL_ROUND

    // Enable binary search / lower_bound by ID
    bool operator<(const StudentRecord& o) const { return id < o.id; }
};

// -------------------------------------------------------
// Per-round aggregated statistics (replaces ~5 maps each)
// -------------------------------------------------------
struct RoundStats {
    int totalCount = 0;

    unordered_map<long long, int>            studentAttempts;   // id  -> count
    unordered_map<long long, vector<string>> studentCompanies;  // id  -> [companies]
    unordered_map<int,       int>            batchAttempts;     // batch -> count
    unordered_map<string,    int>            companyAttempts;   // company -> count
    unordered_map<string,    int>            programAttempts;   // program -> count
    unordered_map<int,       int>            yearAttempts;      // year -> count
};

// -------------------------------------------------------
// Overall (cross-round) statistics kept at class level
// -------------------------------------------------------
struct OverallStats {
    float minPackage   = numeric_limits<float>::max();
    float maxPackage   = numeric_limits<float>::lowest();
    float totalPackage = 0.0f;
    float avgPackage   = 0.0f;

    vector<float>  allPackages;       // for median computation
    vector<string> companiesVisited;  // one entry per company load

    // Student-level offer data
    unordered_map<long long, int>            totalOffers;      // id -> #offers
    unordered_map<long long, vector<string>> offeredCompanies; // id -> [companies]
    unordered_map<long long, vector<float>>  offeredPackages;  // id -> [packages]

    // Aggregate offer counts
    unordered_map<int,    int> batchOffers;
    unordered_map<string, int> programOffers;
    unordered_map<string, int> companyOffers;
    unordered_map<int,    int> yearOffers;
};
