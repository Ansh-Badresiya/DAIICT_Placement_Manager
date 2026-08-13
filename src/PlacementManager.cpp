// ============================================================
//  PlacementManager.cpp  —  Full implementation
//
//  KEY IMPROVEMENTS vs original:
//  1. Single StudentRecord struct (was: Node1 + Node2)
//  2. std::vector per round, binary-search insertion  O(log N)
//     (was: sorted linked list, O(N) per insert → O(N²) bulk)
//  3. One generic filter() + printTable() + writeCSV()
//     (was: hundreds of duplicated display/write functions)
//  4. Index sets for O(1) existence checks
//     (was: O(N) linked-list scan per query)
//  5. Proper header/source split — no #include of .cpp
//  6. computeMedian() takes by VALUE (BUG FIX — original mutated caller's data)
//  7. Proper destructor-free RAII via std::vector (no memory leaks)
// ============================================================

#include "../include/PlacementManager.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <iomanip>
#include <limits>
#include <functional>

using namespace std;

// ============================================================
//  Constructor
// ============================================================
PlacementManager::PlacementManager()
    : rounds_(NUM_ROUNDS), stats_(NUM_ROUNDS)
{
}

// ============================================================
//  PRIVATE — Printing helpers
// ============================================================

void PlacementManager::printHLine(int width, char fill) const
{
    cout << setfill(fill) << setw(width) << "" << setfill(' ') << "\n";
}

void PlacementManager::printTableHeader(bool showPackage) const
{
    printHLine(showPackage ? 177 : 157);
    if (showPackage)
        cout << "|    ID    |        Name        |   Batch  |    Program    |"
                "          Email          |   Contact No  |  WhatsApp No  |"
                "      Company       |    Package    |   Year   |\n";
    else
        cout << "|    ID    |        Name        |   Batch  |    Program    |"
                "          Email          |   Contact No  |  WhatsApp No  |"
                "      Company       |   Year   |\n";
    printHLine(showPackage ? 177 : 157);
}

void PlacementManager::printTableRow(const StudentRecord &r, bool showPackage) const
{
    cout << "|" << setw(10) << left << r.id
         << "|" << setw(20) << left << r.name
         << "|" << setw(10) << left << r.batch
         << "|" << setw(15) << left << r.program
         << "|" << setw(25) << left << r.email
         << "|" << setw(15) << left << r.contactNO
         << "|" << setw(15) << left << r.whatsappNO
         << "|" << setw(20) << left << r.company;
    if (showPackage)
        cout << "|" << setw(15) << left << r.package;
    cout << "|" << setw(10) << left << r.year << "|\n";
}

// Generic table printer — ONE function replaces ~40 in the original
void PlacementManager::printTable(const vector<StudentRecord> &data, bool showPackage) const
{
    printTableHeader(showPackage);
    for (const auto &r : data)
        printTableRow(r, showPackage);
    printHLine(showPackage ? 177 : 157);
}

// ============================================================
//  PRIVATE — CSV writer (ONE function replaces ~40 in original)
// ============================================================
void PlacementManager::writeCSV(const string &path,
                                const vector<StudentRecord> &data,
                                bool showPackage) const
{
    ofstream f(path);
    if (!f.is_open())
    {
        cerr << "\n-----> Error opening file for writing: " << path << "\n\n";
        return;
    }

    // Header row
    f << "Sr.no,ID,Name,Batch,Program,Email,Contact No,WhatsApp No,Company";
    if (showPackage)
        f << ",Package";
    f << ",Year\n";

    int i = 1;
    for (const auto &r : data)
    {
        f << i++ << "," << r.id << "," << r.name << "," << r.batch << ","
          << r.program << "," << r.email << "," << r.contactNO << ","
          << r.whatsappNO << "," << r.company;
        if (showPackage)
            f << "," << r.package;
        f << "," << r.year << "\n";
    }
    cout << "\n-----> Data Written Successfully....\n\n";
}

// ============================================================
//  PRIVATE — Generic filter (ONE function replaces ~80 helpers)
// ============================================================
vector<StudentRecord> PlacementManager::filter(
    Round r,
    function<bool(const StudentRecord &)> pred) const
{
    // Delegate to AVLTree — walks the tree in sorted order
    return rounds_[r].filter(pred);
}

// ============================================================
//  PRIVATE — Not-selected filter (students in R1 with 0 offers)
// ============================================================
vector<StudentRecord> PlacementManager::notSelected(
    function<bool(const StudentRecord &)> pred) const
{
    // Walk R1 tree, keep students who match pred AND have zero job offers
    return rounds_[ROUND_1].filter([&](const StudentRecord &rec)
    {
        return pred(rec) && overall_.totalOffers.count(rec.id) == 0;
    });
}

// ============================================================
//  PRIVATE — Ask display / write
// ============================================================
void PlacementManager::askDisplayWrite(const vector<StudentRecord> &data,
                                       bool showPackage,
                                       const string &header) const
{
    char ch;
    cout << "\n#-----> Do you want to Display Data (Y/N)? \n#-----> Ans: ";
    cin >> ch;
    if (ch == 'Y' || ch == 'y')
    {
        cout << "\n"
             << header << "\n";
        printTable(data, showPackage);
    }

    string path;
    cout << "\n#-----> Enter File Path to store the data: ";
    cin >> path;
    writeCSV(path, data, showPackage);
}

// ============================================================
//  PRIVATE — Ask round (1-5)
// ============================================================
Round PlacementManager::askRound() const
{
    cout << "\n#-----> Select Round (1=R1, 2=R2, 3=R3, 4=R4, 5=Final): ";
    int c;
    cin >> c;
    if (c >= 1 && c <= NUM_ROUNDS)
        return static_cast<Round>(c - 1);
    cout << "\n-----> Invalid round choice.\n";
    return ROUND_1;
}

// ============================================================
//  PRIVATE — Binary-search sorted insert O(log N + shift)
//  Much better than original O(N) linked-list walk
// ============================================================
void PlacementManager::insertSorted(Round r, const StudentRecord &rec)
{
    // True O(log N) AVL insert — replaces the old O(N) vector::insert
    rounds_[r].insert(rec);
}

// ============================================================
//  PRIVATE — Generic CSV loader (replaces 5 near-identical functions)
// ============================================================
void PlacementManager::loadCSV(const string &path, const string &company, Round r)
{
    ifstream file(path);
    if (!file.is_open())
    {
        printHLine(115);
        cerr << "\n-----> Error opening \"" << roundName(r) << "\" file of Company \""
             << company << "\"\n-----> Please try again with a valid path.\n\n";
        printHLine(115);
        return;
    }

    cout << "\n-----------> Fetching data from \"" << roundName(r)
         << "\" file of Company \"" << company << "\" --------------->\n";

    string line;
    getline(file, line); // skip header

    while (getline(file, line))
    {
        if (line.empty())
            continue;
        istringstream ss(line);

        StudentRecord rec;
        string skip, id_str, cno_str, wno_str, date_str, pkg_str;

        getline(ss, skip, ','); // Sr.no
        getline(ss, id_str, ',');
        rec.id = stoll(id_str);
        rec.batch = stoi(id_str.substr(0, 4));
        getline(ss, rec.name, ',');
        getline(ss, rec.program, ',');
        getline(ss, date_str, ',');
        rec.year = stoi(date_str.substr(6, 4));
        getline(ss, rec.email, ',');
        getline(ss, cno_str, ',');
        rec.contactNO = stoll(cno_str);
        getline(ss, wno_str, ',');
        rec.whatsappNO = stoll(wno_str);

        if (r == FINAL_ROUND)
        {
            getline(ss, pkg_str, ',');
            rec.package = stof(pkg_str);
        }

        rec.company = company;

        insertSorted(r, rec);

        // ---- Update statistics ----
        RoundStats &st = stats_[r];
        st.totalCount++;
        st.studentAttempts[rec.id]++;
        st.studentCompanies[rec.id].push_back(company);
        st.batchAttempts[rec.batch]++;
        st.companyAttempts[company]++;
        st.programAttempts[rec.program]++;
        st.yearAttempts[rec.year]++;

        // Update existence index sets (all built from R1 data)
        if (r == ROUND_1)
        {
            batchCompanyIdx_.insert({rec.batch, company});
            batchProgramIdx_.insert({rec.batch, rec.program});
            programCompanyIdx_.insert({rec.program, company});
            yearBatchIdx_.insert({rec.year, rec.batch});
            yearProgramIdx_.insert({rec.year, rec.program});
            yearCompanyIdx_.insert({rec.year, company});

            // Store student info (overwrite is fine — same data)
            studentInfo_[rec.id] = rec;
        }

        if (r == FINAL_ROUND)
        {
            overall_.totalOffers[rec.id]++;
            overall_.offeredCompanies[rec.id].push_back(company);
            overall_.offeredPackages[rec.id].push_back(rec.package);
            overall_.batchOffers[rec.batch]++;
            overall_.programOffers[rec.program]++;
            overall_.companyOffers[company]++;
            overall_.yearOffers[rec.year]++;

            overall_.allPackages.push_back(rec.package);
            if (rec.package < overall_.minPackage)
                overall_.minPackage = rec.package;
            if (rec.package > overall_.maxPackage)
                overall_.maxPackage = rec.package;
            overall_.totalPackage += rec.package;
            overall_.avgPackage = overall_.totalPackage / (float)overall_.allPackages.size();
        }
    }

    if (r == ROUND_1)
        overall_.companiesVisited.push_back(company);

    file.close();
    cout << "<------ Successfully fetched data from \"" << roundName(r)
         << "\" file of Company \"" << company << "\" <--------\n\n";
}

// ============================================================
//  PRIVATE — Median (takes by value — BUG FIX vs original)
// ============================================================
float PlacementManager::computeMedian(vector<float> nums) const
{
    if (nums.empty())
        return 0.0f;
    sort(nums.begin(), nums.end());
    int n = (int)nums.size();
    return (n % 2 == 0) ? (nums[n / 2 - 1] + nums[n / 2]) / 2.0f : nums[n / 2];
}

// ============================================================
//  PRIVATE — Print package stats block
// ============================================================
void PlacementManager::printPackageStats(const vector<StudentRecord> &offered,
                                         int r1Attempts) const
{
    if (offered.empty())
    {
        cout << "\n-----> No offers data available.\n";
        return;
    }
    float mn = numeric_limits<float>::max(), mx = numeric_limits<float>::lowest(), tot = 0;
    vector<float> pkgs;
    for (const auto &r : offered)
    {
        mn = min(mn, r.package);
        mx = max(mx, r.package);
        tot += r.package;
        pkgs.push_back(r.package);
    }
    int offers = (int)offered.size();
    cout << "\nNo. Students Got Job Offer        : " << offers;
    if (r1Attempts > 0)
        cout << "\nSuccess Rate                      : "
             << fixed << setprecision(2) << (float)offers / r1Attempts * 100 << "%";
    cout << "\n\nMaximum Package Offered           : " << mx << " LPA"
         << "\nMinimum Package Offered           : " << mn << " LPA"
         << "\nAverage Package                   : " << tot / offers << " LPA"
         << "\nMedian  Package                   : " << computeMedian(pkgs) << " LPA\n";
}

// ============================================================
//  PRIVATE — Existence checks (O(1) set lookup)
// ============================================================
bool PlacementManager::isDataLoaded() const
{
    for (int i = 0; i < NUM_ROUNDS; ++i)
        if (rounds_[i].empty())
            return false;
    return true;
}
bool PlacementManager::batchExists(int b) const
{
    return stats_[ROUND_1].batchAttempts.count(b) && stats_[ROUND_1].batchAttempts.at(b) > 0;
}
bool PlacementManager::programExists(const string &p) const
{
    return stats_[ROUND_1].programAttempts.count(p) && stats_[ROUND_1].programAttempts.at(p) > 0;
}
bool PlacementManager::companyExists(const string &c) const
{
    return stats_[ROUND_1].companyAttempts.count(c) && stats_[ROUND_1].companyAttempts.at(c) > 0;
}
bool PlacementManager::yearExists(int y) const
{
    return stats_[ROUND_1].yearAttempts.count(y) && stats_[ROUND_1].yearAttempts.at(y) > 0;
}

bool PlacementManager::batchCompanyExists(int b, const string &c) const
{
    return batchCompanyIdx_.count({b, c});
}
bool PlacementManager::batchProgramExists(int b, const string &p) const
{
    return batchProgramIdx_.count({b, p});
}
bool PlacementManager::programCompanyExists(const string &p, const string &c) const
{
    return programCompanyIdx_.count({p, c});
}
bool PlacementManager::yearBatchExists(int y, int b) const
{
    return yearBatchIdx_.count({y, b});
}
bool PlacementManager::yearProgramExists(int y, const string &p) const
{
    return yearProgramIdx_.count({y, p});
}
bool PlacementManager::yearCompanyExists(int y, const string &c) const
{
    return yearCompanyIdx_.count({y, c});
}

// ============================================================
// ============================================================
//  PUBLIC — InputPlacementData
// ============================================================
// ============================================================
void PlacementManager::InputPlacementData()
{
    cin.ignore();
    string company;
    cout << "\n#-----> Enter Company's Name: ";
    getline(cin, company);

    cout << "\n\n<------------------------------------------------------>\n"
         << "\n---> Fetching data from files of Company \"" << company << "\"\n"
         << "<------------------------------------------------------>\n\n";

    for (int r = 0; r < NUM_ROUNDS; ++r)
    {
        string path;
        cout << "\n#-----> Enter file path for " << roundName(r) << ": ";
        getline(cin, path);
        loadCSV(path, company, static_cast<Round>(r));
    }
}

// ============================================================
// ============================================================
//  SORT / DISPLAY / EXPORT  — Public methods
//
//  Each function:
//   1. Validates data is loaded
//   2. Reads filter criterion from user
//   3. Validates criterion exists in dataset
//   4. Asks which round
//   5. Calls filter() with a lambda predicate → vector<StudentRecord>
//   6. Calls askDisplayWrite() for display + CSV export
// ============================================================
// ============================================================

// --- Helper macro to avoid repetition of the round-loop pattern ---
#define SORT_WITH_FILTER(predicate, header_str, is_fr)      \
    do                                                      \
    {                                                       \
        Round rd = askRound();                              \
        bool isFR = (rd == FINAL_ROUND);                    \
        auto data = filter(rd, predicate);                  \
        askDisplayWrite(data, isFR || (is_fr), header_str); \
    } while (0)

void PlacementManager::SortWholeData()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded. Please insert data first.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = rounds_[rd].toVector(); // get all records in sorted order
    askDisplayWrite(data, isFR, "All Data - " + roundName(rd));
}

void PlacementManager::SortDataBatchWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    if (!batchExists(batch))
    {
        cout << "\n-----> Batch " << batch << " not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.batch == batch; });
    askDisplayWrite(data, isFR, "Batch " + to_string(batch) + " — " + roundName(rd));
}

void PlacementManager::SortDataProgramWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!programExists(prog))
    {
        cout << "\n-----> Program '" << prog << "' not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.program == prog; });
    askDisplayWrite(data, isFR, "Program " + prog + " — " + roundName(rd));
}

void PlacementManager::SortDataYearWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    if (!yearExists(year))
    {
        cout << "\n-----> Year " << year << " not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.year == year; });
    askDisplayWrite(data, isFR, "Year " + to_string(year) + " — " + roundName(rd));
}

void PlacementManager::SortDataCompanyWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!companyExists(comp))
    {
        cout << "\n-----> Company '" << comp << "' not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.company == comp; });
    askDisplayWrite(data, isFR, comp + " — " + roundName(rd));
}

void PlacementManager::SortDataBatchAndCompanyWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!batchCompanyExists(batch, comp))
    {
        cout << "\n-----> Batch " << batch << " + Company '" << comp << "' not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.batch == batch && r.company == comp; });
    askDisplayWrite(data, isFR, "Batch " + to_string(batch) + " + " + comp + " — " + roundName(rd));
}

void PlacementManager::SortDataProgramOFBatchWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!batchProgramExists(batch, prog))
    {
        cout << "\n-----> Batch " << batch << " + Program '" << prog << "' not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.batch == batch && r.program == prog; });
    askDisplayWrite(data, isFR, "Batch " + to_string(batch) + " + " + prog + " — " + roundName(rd));
}

void PlacementManager::SortDataProgramOFCompanyWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string prog, comp;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!programCompanyExists(prog, comp))
    {
        cout << "\n-----> Program '" << prog << "' + Company '" << comp << "' not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.program == prog && r.company == comp; });
    askDisplayWrite(data, isFR, prog + " + " + comp + " — " + roundName(rd));
}

void PlacementManager::SortDataProgramOFYearWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!yearProgramExists(year, prog))
    {
        cout << "\n-----> Year " << year << " + Program '" << prog << "' not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.year == year && r.program == prog; });
    askDisplayWrite(data, isFR, to_string(year) + " + " + prog + " — " + roundName(rd));
}

void PlacementManager::SortDataYearAndBatchWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year, batch;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    if (!yearBatchExists(year, batch))
    {
        cout << "\n-----> Year " << year << " + Batch " << batch << " not found.\n";
        return;
    }
    Round rd = askRound();
    bool isFR = (rd == FINAL_ROUND);
    auto data = filter(rd, [&](const StudentRecord &r)
                       { return r.year == year && r.batch == batch; });
    askDisplayWrite(data, isFR, to_string(year) + " + Batch " + to_string(batch) + " — " + roundName(rd));
}

// ============================================================
// ============================================================
//  PLACEMENT STATISTICS
// ============================================================
// ============================================================

void PlacementManager::FindOverallPlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cout << "\n";
    printHLine(60);
    cout << "\n# Overall Placement Statistics:\n";
    for (int r = 0; r < NUM_ROUNDS - 1; ++r)
        cout << "\nNo. Students in " << roundName(r) << "  : " << stats_[r].totalCount;
    cout << "\nNo. Students Got Job Offer    : " << stats_[FINAL_ROUND].totalCount;
    cout << "\nSuccess Rate                  : "
         << fixed << setprecision(2)
         << (float)stats_[FINAL_ROUND].totalCount / stats_[ROUND_1].totalCount * 100 << "%";
    cout << "\n\nMaximum Package Offered       : " << overall_.maxPackage << " LPA"
         << "\nMinimum Package Offered       : " << overall_.minPackage << " LPA"
         << "\nAverage Package               : " << overall_.avgPackage << " LPA"
         << "\nMedian  Package               : " << computeMedian(overall_.allPackages) << " LPA\n\n";
    printHLine(150);
    cout << "\nNo. Of Companies Visited : " << overall_.companiesVisited.size();
    cout << "\n\nCompanies:\n\n";
    for (size_t i = 0; i < overall_.companiesVisited.size(); ++i)
    {
        cout << overall_.companiesVisited[i] << " , ";
        if ((i + 1) % 5 == 0)
            cout << "\n";
    }
    cout << "\n";
    printHLine(150);
}

void PlacementManager::FindStudentPlacementDetails()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    long long id;
    cout << "\n#-----> Enter Student's ID: ";
    cin >> id;
    if (!studentInfo_.count(id))
    {
        cout << "\n-----> Invalid ID. Student not found.\n";
        return;
    }
    const StudentRecord &s = studentInfo_.at(id);
    cout << "\n";
    printHLine(60);
    cout << "\n# " << id << "'s Placement Details:\n";
    cout << "\nName           : " << s.name
         << "\nID             : " << id
         << "\nBatch          : " << s.batch
         << "\nProgram        : " << s.program
         << "\nEmail          : " << s.email
         << "\nContact No     : " << s.contactNO
         << "\nWhatsApp No    : " << s.whatsappNO;
    int offers = overall_.totalOffers.count(id) ? overall_.totalOffers.at(id) : 0;
    int r1att = stats_[ROUND_1].studentAttempts.count(id) ? stats_[ROUND_1].studentAttempts.at(id) : 1;
    cout << "\nSuccess Rate   : " << fixed << setprecision(2)
         << (float)offers / r1att * 100 << "%\n\n";
    printHLine(100);

    for (int r = 0; r < NUM_ROUNDS - 1; ++r)
    {
        int att = stats_[r].studentAttempts.count(id) ? stats_[r].studentAttempts.at(id) : 0;
        cout << "\nNo. of Attempts in " << roundName(r) << " : " << att;
        if (stats_[r].studentCompanies.count(id))
        {
            cout << "\nCompanies : ";
            for (const auto &c : stats_[r].studentCompanies.at(id))
                cout << c << " , ";
        }
    }
    cout << "\n\nNo. of Job Offers : " << offers;
    if (overall_.offeredCompanies.count(id))
    {
        cout << "\nCompanies : ";
        for (const auto &c : overall_.offeredCompanies.at(id))
            cout << c << " , ";
        cout << "\nPackages [LPA] : ";
        for (float p : overall_.offeredPackages.at(id))
            cout << p << " , ";
    }
    cout << "\n\n";
    printHLine(100);
}

// ---- Helper: compute stats from a filtered FR vector ----
static void statsBlock(const vector<StudentRecord> &fr,
                       int r1att, int r2att, int r3att, int r4att,
                       const string &label,
                       function<float(vector<float>)> medFn)
{
    float mn = numeric_limits<float>::max(), mx = numeric_limits<float>::lowest(), tot = 0;
    vector<float> pkgs;
    set<string> uniq;
    for (const auto &r : fr)
    {
        mn = min(mn, r.package);
        mx = max(mx, r.package);
        tot += r.package;
        pkgs.push_back(r.package);
        uniq.insert(r.program + " - " + r.company);
    }
    int n = (int)fr.size();
    cout << "\n"; // horizontal line printed by caller
    cout << "\n# " << label << "\n";
    cout << "\nNo. Students in Round 1 : " << r1att
         << "\nNo. Students in Round 2 : " << r2att
         << "\nNo. Students in Round 3 : " << r3att
         << "\nNo. Students in Round 4 : " << r4att
         << "\nNo. Students Got Offer  : " << n;
    if (r1att > 0)
        cout << "\nSuccess Rate            : " << fixed << setprecision(2)
             << (float)n / r1att * 100 << "%";
    if (n > 0)
        cout << "\n\nMax Package [LPA] : " << mx
             << "\nMin Package [LPA] : " << mn
             << "\nAvg Package [LPA] : " << tot / n
             << "\nMedian      [LPA] : " << medFn(pkgs);
    cout << "\n\nUnique Program-Company combos:\n";
    int i = 0;
    for (const auto &s : uniq)
    {
        cout << s << " , ";
        if (++i % 5 == 0)
            cout << "\n";
    }
    cout << "\n";
}

void PlacementManager::FindBatchWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    if (!batchExists(batch))
    {
        cout << "\n-----> Batch not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.batch == batch; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Batch " + to_string(batch) + " Placement Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindProgramWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!programExists(prog))
    {
        cout << "\n-----> Program not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.program == prog; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Program " + prog + " Placement Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindCompanyWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!companyExists(comp))
    {
        cout << "\n-----> Company not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.company == comp; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               comp + " Placement Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindYearWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    if (!yearExists(year))
    {
        cout << "\n-----> Year not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.year == year; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Year " + to_string(year) + " Placement Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindBatchAndCompanyWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!batchCompanyExists(batch, comp))
    {
        cout << "\n-----> Batch+Company not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.batch == batch && r.company == comp; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Batch " + to_string(batch) + " + " + comp + " Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindProgramAndBatchWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!batchProgramExists(batch, prog))
    {
        cout << "\n-----> Batch+Program not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.batch == batch && r.program == prog; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Batch " + to_string(batch) + " + " + prog + " Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindProgramAndCompanyWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string prog, comp;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!programCompanyExists(prog, comp))
    {
        cout << "\n-----> Program+Company not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.program == prog && r.company == comp; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               prog + " + " + comp + " Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindYearAndBatchWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year, batch;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    if (!yearBatchExists(year, batch))
    {
        cout << "\n-----> Year+Batch not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.year == year && r.batch == batch; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Year " + to_string(year) + " + Batch " + to_string(batch) + " Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindYearAndCompanyWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!yearCompanyExists(year, comp))
    {
        cout << "\n-----> Year+Company not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.year == year && r.company == comp; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Year " + to_string(year) + " + " + comp + " Statistics", fn);
    printHLine(150);
}

void PlacementManager::FindYearAndProgramWisePlacementStatistics()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!yearProgramExists(year, prog))
    {
        cout << "\n-----> Year+Program not found.\n";
        return;
    }

    auto pred = [&](const StudentRecord &r)
    { return r.year == year && r.program == prog; };
    auto fn = [this](vector<float> v)
    { return computeMedian(v); };
    printHLine(60);
    statsBlock(filter(FINAL_ROUND, pred),
               (int)filter(ROUND_1, pred).size(),
               (int)filter(ROUND_2, pred).size(),
               (int)filter(ROUND_3, pred).size(),
               (int)filter(ROUND_4, pred).size(),
               "Year " + to_string(year) + " + " + prog + " Statistics", fn);
    printHLine(150);
}

// ============================================================
// ============================================================
//  NOT-SELECTED STUDENTS
//
//  Pattern: scan R1, keep only students with totalOffers == 0
//  that match the filter predicate, then display+export.
// ============================================================
// ============================================================

// Common helper for not-selected display + write
static void nsDisplayWrite(PlacementManager *pm,
                           const vector<StudentRecord> &data,
                           const string &label)
{
    char ch;
    cout << "\n#-----> Do you want to Display Data (Y/N)? \n#-----> Ans: ";
    cin >> ch;
    if (ch == 'Y' || ch == 'y')
    {
        cout << "\n<--- Not Selected: " << label << " --->\n";
    }
    // reuse PlacementManager's display via callback pattern below
    (void)pm;
    (void)data; // display done inline
}

void PlacementManager::FindNotSelectedBatchWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    if (!batchExists(batch))
    {
        cout << "\n-----> Batch not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.batch == batch; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All students of Batch " << batch << " are placed!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Batch " + to_string(batch));
}

void PlacementManager::FindNotSelectedProgramWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!programExists(prog))
    {
        cout << "\n-----> Program not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.program == prog; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All students of Program " << prog << " are placed!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — " + prog);
}

void PlacementManager::FindNotSelectedCompanyWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!companyExists(comp))
    {
        cout << "\n-----> Company not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.company == comp; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All students of Company " << comp << " are placed!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — " + comp);
}

void PlacementManager::FindNotSelectedYearWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    if (!yearExists(year))
    {
        cout << "\n-----> Year not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.year == year; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All students of Year " << year << " are placed!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Year " + to_string(year));
}

void PlacementManager::FindNotSelectedBatchAndProgramWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!batchProgramExists(batch, prog))
    {
        cout << "\n-----> Batch+Program not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.batch == batch && r.program == prog; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All placed in Batch " << batch << " + " << prog << "!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Batch " + to_string(batch) + " + " + prog);
}

void PlacementManager::FindNotSelectedBatchAndCompanyWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int batch;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!batchCompanyExists(batch, comp))
    {
        cout << "\n-----> Batch+Company not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.batch == batch && r.company == comp; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All placed in Batch " << batch << " + " << comp << "!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Batch " + to_string(batch) + " + " + comp);
}

void PlacementManager::FindNotSelectedCompanyAndProgramWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    cin.ignore();
    string comp, prog;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!programCompanyExists(prog, comp))
    {
        cout << "\n-----> Program+Company not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.program == prog && r.company == comp; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All placed in " << prog << " + " << comp << "!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — " + prog + " + " + comp);
}

void PlacementManager::FindNotSelectedYearAndBatchWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year, batch;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cout << "\n#-----> Enter Batch: ";
    cin >> batch;
    if (!yearBatchExists(year, batch))
    {
        cout << "\n-----> Year+Batch not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.year == year && r.batch == batch; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All placed in Year " << year << " + Batch " << batch << "!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Year " + to_string(year) + " + Batch " + to_string(batch));
}

void PlacementManager::FindNotSelectedYearAndProgramWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cin.ignore();
    string prog;
    cout << "\n#-----> Enter Program: ";
    getline(cin, prog);
    if (!yearProgramExists(year, prog))
    {
        cout << "\n-----> Year+Program not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.year == year && r.program == prog; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All placed in Year " << year << " + " << prog << "!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Year " + to_string(year) + " + " + prog);
}

void PlacementManager::FindNotSelectedYearAndCompanyWise()
{
    if (!isDataLoaded())
    {
        cout << "\n-----> No data loaded.\n";
        return;
    }
    int year;
    cout << "\n#-----> Enter Year: ";
    cin >> year;
    cin.ignore();
    string comp;
    cout << "\n#-----> Enter Company Name: ";
    getline(cin, comp);
    if (!yearCompanyExists(year, comp))
    {
        cout << "\n-----> Year+Company not found.\n";
        return;
    }
    auto data = notSelected([&](const StudentRecord &r)
                            { return r.year == year && r.company == comp; });
    if (data.empty())
    {
        cout << "\n---> Wow!!! All placed in Year " << year << " + " << comp << "!\n";
        return;
    }
    askDisplayWrite(data, false, "Not Selected — Year " + to_string(year) + " + " + comp);
}
