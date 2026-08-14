// ============================================================
//  PlacementManager.h  —  Class declaration
//  All public methods mirror the original interface exactly.
//  Internal storage is completely redesigned.
// ============================================================
#pragma once

#include "StudentRecord.h"
#include "AVLTree.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <set>
#include <iomanip>

using namespace std;

class PlacementManager
{
    // ===================================================================
    // DATA STORAGE
    // ===================================================================
    // rounds_[r] holds every record loaded for round r.
    // AVLTree gives true O(log N) insert + cache-friendly traversal.
    vector<AVLTree<StudentRecord>> rounds_;   // [Round] -> one AVL tree per round

    // Quick student-info lookup (populated from R1; replaces 6 separate maps)
    unordered_map<long long, StudentRecord> studentInfo_;

    // Per-round statistics (replaces 5 maps x 5 rounds = 25 maps)
    vector<RoundStats> stats_;     // stats_[Round]

    // Cross-round / offer statistics
    OverallStats overall_;

    // ===================================================================
    // FAST EXISTENCE-CHECK INDEX SETS (O(1) vs O(N) linked-list scan)
    // ===================================================================
    set<pair<int,    string>> batchCompanyIdx_;    // {batch, company}
    set<pair<int,    string>> batchProgramIdx_;    // {batch, program}
    set<pair<string, string>> programCompanyIdx_;  // {program, company}
    set<pair<int,    int>>    yearBatchIdx_;        // {year, batch}
    set<pair<int,    string>> yearProgramIdx_;      // {year, program}
    set<pair<int,    string>> yearCompanyIdx_;      // {year, company}

    // ===================================================================
    // PRIVATE HELPERS
    // ===================================================================
    // Table printing / writing
    void printHLine(int width, char fill = '-') const;
    void printTableHeader(bool showPackage) const;
    void printTableRow(const StudentRecord& r, bool showPackage) const;
    void printTable(const vector<StudentRecord>& data, bool showPackage) const;
    void writeCSV(const string& path,
                  const vector<StudentRecord>& data,
                  bool showPackage) const;

    // Generic filter — returns matching records from round r
    vector<StudentRecord> filter(
        Round r,
        function<bool(const StudentRecord&)> pred) const;

    // "Not-selected" filter — scans R1 for students with totalOffers == 0
    vector<StudentRecord> notSelected(
        function<bool(const StudentRecord&)> pred) const;

    // Asks user: display? then asks for output file path.
    void askDisplayWrite(const vector<StudentRecord>& data,
                         bool showPackage,
                         const string& header) const;

    // Asks user for round choice (1-5); returns Round enum value.
    Round askRound() const;

    // Inserts a record into rounds_[r] in sorted (by ID) order via AVL Tree.
    void insertSorted(Round r, const StudentRecord& rec);

    // Reads one CSV file and populates round r data + statistics.
    void loadCSV(const string& path, const string& company, Round r);

    // Computes median without mutating the source vector (BUG FIX).
    float computeMedian(vector<float> nums) const;

    // Stats helpers
    void printPackageStats(const vector<StudentRecord>& offered,
                           int r1Attempts) const;

    // Existence checks — O(1) via index sets (vs O(N) in original)
    bool isDataLoaded() const;
    bool batchExists(int batch)          const;
    bool programExists(const string& p)  const;
    bool companyExists(const string& c)  const;
    bool yearExists(int year)            const;

    bool batchCompanyExists(int batch, const string& company)     const;
    bool batchProgramExists(int batch, const string& program)     const;
    bool programCompanyExists(const string& p, const string& c)   const;
    bool yearBatchExists(int year, int batch)                      const;
    bool yearProgramExists(int year, const string& program)        const;
    bool yearCompanyExists(int year, const string& company)        const;

public:
    // Constructor
    PlacementManager();

    // ===================================================================
    // PUBLIC INTERFACE 
    // ===================================================================

    // --- Data Input ---
    void InputPlacementData();

    // --- Sort / Display / Export ---
    void SortWholeData();
    void SortDataBatchWise();
    void SortDataProgramWise();
    void SortDataYearWise();
    void SortDataCompanyWise();
    void SortDataBatchAndCompanyWise();
    void SortDataProgramOFBatchWise();
    void SortDataProgramOFCompanyWise();
    void SortDataProgramOFYearWise();
    void SortDataYearAndBatchWise();

    // --- Placement Statistics ---
    void FindOverallPlacementStatistics();
    void FindStudentPlacementDetails();
    void FindBatchWisePlacementStatistics();
    void FindProgramWisePlacementStatistics();
    void FindCompanyWisePlacementStatistics();
    void FindYearWisePlacementStatistics();
    void FindBatchAndCompanyWisePlacementStatistics();
    void FindProgramAndBatchWisePlacementStatistics();
    void FindProgramAndCompanyWisePlacementStatistics();
    void FindYearAndBatchWisePlacementStatistics();
    void FindYearAndCompanyWisePlacementStatistics();
    void FindYearAndProgramWisePlacementStatistics();

    // --- Not-Selected Students ---
    void FindNotSelectedBatchWise();
    void FindNotSelectedProgramWise();
    void FindNotSelectedCompanyWise();
    void FindNotSelectedYearWise();
    void FindNotSelectedBatchAndProgramWise();
    void FindNotSelectedBatchAndCompanyWise();
    void FindNotSelectedCompanyAndProgramWise();
    void FindNotSelectedYearAndBatchWise();
    void FindNotSelectedYearAndProgramWise();
    void FindNotSelectedYearAndCompanyWise();
};
