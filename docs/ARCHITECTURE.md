# System Architecture — DAIICT Placement Manager

> This document provides a detailed technical deep-dive into the system architecture, component responsibilities, data structures, and memory layout of the Placement Manager.

---

## Table of Contents

- [High-Level Architecture](#high-level-architecture)
- [Layer Breakdown](#layer-breakdown)
  - [Presentation Layer — main.cpp](#1-presentation-layer--maincpp)
  - [Business Logic Layer — PlacementManager](#2-business-logic-layer--placementmanager)
  - [Data Structure Layer — AVLTree](#3-data-structure-layer--avltree)
  - [Data Model Layer — StudentRecord.h](#4-data-model-layer--studentrecordh)
- [Internal Storage Design](#internal-storage-design)
  - [Per-Round Storage (AVL Trees)](#per-round-storage-avl-trees)
  - [Student Info Lookup Map](#student-info-lookup-map)
  - [Per-Round Statistics (RoundStats)](#per-round-statistics-roundstats)
  - [Cross-Round Statistics (OverallStats)](#cross-round-statistics-overallstats)
  - [Existence Index Sets](#existence-index-sets)
- [Key Algorithms](#key-algorithms)
  - [CSV Loading Pipeline](#csv-loading-pipeline)
  - [Generic Filter Engine](#generic-filter-engine)
  - [Not-Selected Detection](#not-selected-detection)
  - [Statistics Computation](#statistics-computation)
- [Memory Layout](#memory-layout)
- [Complexity Analysis](#complexity-analysis)

---

## High-Level Architecture

The system follows a **3-layer console architecture**:

```
╔══════════════════════════════════════════════════════════════════╗
║                    PRESENTATION LAYER                            ║
║                     main.cpp                                     ║
║  displayMainMenu()  displaySortMenu()  displayStatisticsMenu()   ║
║  displayNotSelectedMenu()                                        ║
╚══════════════════════════╦═══════════════════════════════════════╝
                           ║  method calls (PlacementManager& p)
╔══════════════════════════╩═══════════════════════════════════════╗
║                   BUSINESS LOGIC LAYER                           ║
║                    PlacementManager                              ║
║                                                                  ║
║  ┌─────────────┐  ┌──────────────┐  ┌────────────────────────┐  ║
║  │  Data Input │  │Sort & Filter │  │  Statistics & Reports  │  ║
║  │ InputData() │  │ SortXxx()    │  │  FindXxxStatistics()   │  ║
║  │  loadCSV()  │  │  filter()    │  │  FindNotSelected()     │  ║
║  └─────────────┘  └──────────────┘  └────────────────────────┘  ║
╚══════════════════════════╦═══════════════════════════════════════╝
                           ║  stores/retrieves data
╔══════════════════════════╩═══════════════════════════════════════╗
║                     DATA LAYER                                   ║
║                                                                  ║
║  AVLTree<StudentRecord>[5]    unordered_map<id, StudentRecord>   ║
║  RoundStats[5]                OverallStats                       ║
║  set<pair<...>>[6]  ← existence index sets                       ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## Layer Breakdown

### 1. Presentation Layer — `main.cpp`

**Responsibility:** Menu rendering and user input routing only.

```
main()
  └── do-while loop
        ├── displayMainMenu()          → prints main menu, reads choice
        ├── case 1: p.InputPlacementData()
        ├── case 2: displaySortMenu(p)
        │             └── reads sub-choice → calls p.SortXxx()
        ├── case 3: displayStatisticsMenu(p)
        │             └── reads sub-choice → calls p.FindXxxStatistics()
        ├── case 4: displayNotSelectedMenu(p)
        │             └── reads sub-choice → calls p.FindNotSelectedXxx()
        └── case 5: Exit
```

The presentation layer has **zero knowledge of data structures** — it only holds a single `PlacementManager p` object and delegates all logic to it.

---

### 2. Business Logic Layer — `PlacementManager`

**Responsibility:** All business rules, data loading, filtering, statistics computation, and output.

PlacementManager is organized into **5 functional groups**:

#### Group 1: Data Input
- `InputPlacementData()` — prompts for company name + 5 CSV paths, calls `loadCSV()` for each round
- `loadCSV(path, company, round)` — parses CSV line by line, calls `insertSorted()`, updates all stats structures

#### Group 2: Sorting / Display / Export (10 methods)
- `SortWholeData()`, `SortDataBatchWise()`, ..., `SortDataYearAndBatchWise()`
- **Pattern:** Validate → get filter criterion from user → validate criterion → ask round → `filter()` → `askDisplayWrite()`

#### Group 3: Placement Statistics (12 methods)
- `FindOverallPlacementStatistics()`, `FindBatchWisePlacementStatistics()`, ...
- **Pattern:** Validate → get criterion → validate → call `statsBlock()` helper with 5-round counts + Final Round data

#### Group 4: Not-Selected Students (10 methods)
- `FindNotSelectedBatchWise()`, `FindNotSelectedProgramWise()`, ...
- **Pattern:** Validate → get criterion → validate → `notSelected(lambda)` → `askDisplayWrite()`

#### Group 5: Private Helpers
- `printHLine()`, `printTableHeader()`, `printTableRow()`, `printTable()` — table rendering
- `writeCSV()` — CSV export
- `filter()`, `notSelected()` — generic data retrieval
- `askDisplayWrite()`, `askRound()` — user interaction helpers
- `insertSorted()` — delegates to `AVLTree::insert()`
- `computeMedian()` — median calculation (takes by value to avoid mutation)
- `printPackageStats()` — salary statistics block
- `isDataLoaded()`, `batchExists()`, `programExists()`, ... — O(1) validation checks

---

### 3. Data Structure Layer — `AVLTree<T>`

**Responsibility:** Maintain student records in sorted order with guaranteed O(log N) insertion.

Key design decisions:
- **Array-indexed nodes** — all nodes live in a `std::vector<Node>` (no heap allocation per node)
- **Integer pointers** — `left` and `right` are `int` indices into the vector, not raw pointers
- **Cache-friendly** — contiguous memory layout improves in-order traversal performance
- **Template** — works for any type T that has `operator<` defined

```
AVLTree<StudentRecord>
├── nodes: vector<Node>           ← all nodes in one contiguous allocation
│   └── Node { data: T, left: int, right: int, height: int }
├── root: int                     ← index of root node (-1 if empty)
│
├── Public API
│   ├── insert(data)    → O(log N), auto-balances via rotations
│   ├── toVector()      → O(N), in-order traversal → sorted vector
│   ├── filter(pred)    → O(N), in-order + predicate filter
│   ├── empty()         → O(1)
│   └── size()          → O(1)
│
└── Private (Balance)
    ├── getHeight(i)
    ├── updateHeight(i)
    ├── getBalance(i)   ← balance factor = height(left) - height(right)
    ├── rotateLeft(x)   ← right-right imbalance fix
    ├── rotateRight(y)  ← left-left imbalance fix
    └── insertAt(i, data) ← recursive insert + 4 rotation cases
```

**Four AVL Rotation Cases:**

```
Case 1 — Left-Left (bf > 1, new data < left child's data):
    rotateRight(current)

Case 2 — Right-Right (bf < -1, new data > right child's data):
    rotateLeft(current)

Case 3 — Left-Right (bf > 1, new data > left child's data):
    rotateLeft(left child) → rotateRight(current)

Case 4 — Right-Left (bf < -1, new data < right child's data):
    rotateRight(right child) → rotateLeft(current)
```

---

### 4. Data Model Layer — `StudentRecord.h`

**Responsibility:** Define all data structures, enums, and constants.

```
StudentRecord.h
├── enum Round          ← ROUND_1=0, ROUND_2=1, ROUND_3=2, ROUND_4=3, FINAL_ROUND=4, NUM_ROUNDS=5
├── roundName(int r)    ← helper returning "Round 1", ..., "Final Round"
├── struct StudentRecord
│   ├── id, name, batch, program, email, contactNO, whatsappNO
│   ├── company, year
│   ├── package         ← only meaningful in FINAL_ROUND
│   └── operator<       ← compares by id (enables AVL tree ordering)
├── struct RoundStats
│   ├── totalCount
│   ├── studentAttempts   : unordered_map<id, int>
│   ├── studentCompanies  : unordered_map<id, vector<string>>
│   ├── batchAttempts     : unordered_map<batch, int>
│   ├── companyAttempts   : unordered_map<company, int>
│   ├── programAttempts   : unordered_map<program, int>
│   └── yearAttempts      : unordered_map<year, int>
└── struct OverallStats
    ├── minPackage, maxPackage, totalPackage, avgPackage
    ├── allPackages       : vector<float>  ← for median
    ├── companiesVisited  : vector<string>
    ├── totalOffers       : unordered_map<id, int>
    ├── offeredCompanies  : unordered_map<id, vector<string>>
    ├── offeredPackages   : unordered_map<id, vector<float>>
    ├── batchOffers       : unordered_map<batch, int>
    ├── programOffers     : unordered_map<program, int>
    ├── companyOffers     : unordered_map<company, int>
    └── yearOffers        : unordered_map<year, int>
```

---

## Internal Storage Design

### Per-Round Storage (AVL Trees)

```cpp
vector<AVLTree<StudentRecord>> rounds_;  // rounds_[Round enum value]
```

One `AVLTree<StudentRecord>` per round. Each tree stores all student records for that round across **all companies loaded so far**. Records are ordered by `StudentRecord::id` (i.e., by student ID).

When multiple companies are loaded, their records intermix in the same tree, sorted by student ID. The `company` field in each record identifies which company the record belongs to.

### Student Info Lookup Map

```cpp
unordered_map<long long, StudentRecord> studentInfo_;
```

Populated **only from Round 1 data**. Provides O(1) lookup of a student's base information (name, program, email, contact) given their ID. Used by `FindStudentPlacementDetails()`.

### Per-Round Statistics (RoundStats)

```cpp
vector<RoundStats> stats_;  // stats_[Round enum value]
```

Updated during `loadCSV()`. Each `RoundStats` stores aggregate counts for: total records, per-student attempts, per-student companies, per-batch counts, per-company counts, per-program counts, per-year counts.

### Cross-Round Statistics (OverallStats)

```cpp
OverallStats overall_;
```

Updated **only from Final Round data**. Tracks all offer-related information: total offers per student, companies that made offers, packages offered, and aggregated offer counts by batch/program/company/year.

### Existence Index Sets

```cpp
set<pair<int,    string>> batchCompanyIdx_;    // {batch, company}
set<pair<int,    string>> batchProgramIdx_;    // {batch, program}
set<pair<string, string>> programCompanyIdx_;  // {program, company}
set<pair<int,    int>>    yearBatchIdx_;        // {year, batch}
set<pair<int,    string>> yearProgramIdx_;      // {year, program}
set<pair<int,    string>> yearCompanyIdx_;      // {year, company}
```

Populated during Round 1 loading. Each set allows O(1) `count()` checks for composite filter validity — e.g., "does batch 2021 have students in Company1?" — replacing O(N) linear scans used in the original.

---

## Key Algorithms

### CSV Loading Pipeline

```
loadCSV(path, company, round r)
│
├── Open file, skip header row
├── For each data row:
│   ├── Parse: Sr.No, id, name, program, date(→year), email, contactNO, whatsappNO
│   ├── If FINAL_ROUND: also parse package
│   ├── Set rec.company = company
│   ├── insertSorted(r, rec)          → AVLTree::insert() → O(log N)
│   ├── Update stats_[r]:
│   │   ├── totalCount++
│   │   ├── studentAttempts[id]++
│   │   ├── studentCompanies[id].push_back(company)
│   │   ├── batchAttempts[batch]++
│   │   ├── companyAttempts[company]++
│   │   ├── programAttempts[program]++
│   │   └── yearAttempts[year]++
│   ├── If ROUND_1: populate 6 index sets + studentInfo_
│   └── If FINAL_ROUND: update overall_ (offers, packages, aggregates)
└── If ROUND_1: append to overall_.companiesVisited
```

**Batch derivation:** `batch = stoi(id_str.substr(0, 4))` — the first 4 digits of the student ID are the admission year/batch.

**Year derivation:** `year = stoi(date_str.substr(6, 4))` — the interview date column is in `DD-MM-YYYY` format; year is at position 6.

### Generic Filter Engine

```cpp
// PlacementManager::filter() — the heart of all Sort/Statistics queries
vector<StudentRecord> filter(Round r, function<bool(const StudentRecord&)> pred) const {
    return rounds_[r].filter(pred);
}

// AVLTree::filter() — in-order traversal collecting matching records
vector<T> filter(const function<bool(const T&)>& pred) const {
    vector<T> result;
    filterAt(root, pred, result);
    return result;
}
```

Every Sort and Statistics method uses this single generic filter with a **lambda predicate**. Examples:

```cpp
// Batch-wise filter
filter(rd, [&](const StudentRecord& r) { return r.batch == batch; });

// Batch + Company composite filter
filter(rd, [&](const StudentRecord& r) { return r.batch == batch && r.company == comp; });
```

### Not-Selected Detection

```cpp
vector<StudentRecord> notSelected(function<bool(const StudentRecord&)> pred) const {
    // Scans Round 1 tree, keeps students who:
    //   1. Match the predicate (batch/program/year/etc.)
    //   2. Have totalOffers == 0 (not in overall_.totalOffers map)
    return rounds_[ROUND_1].filter([&](const StudentRecord& rec) {
        return pred(rec) && overall_.totalOffers.count(rec.id) == 0;
    });
}
```

Round 1 is used as the **universe** — everyone who appeared for at least one company's Round 1 is considered a "participant." Those with `totalOffers == 0` are "not selected."

### Statistics Computation

The `statsBlock()` static helper computes and prints a full statistics block for any filtered subset:

```
Input: Final Round records + R1/R2/R3/R4 attempt counts + label

Output:
  - No. of students per round
  - No. of students who got offer
  - Success rate (%)
  - Max/Min/Average/Median package (LPA)
  - Unique Program-Company combinations
```

Median is computed via `computeMedian(vector<float> nums)` which **sorts a copy** of the data — this is the key bug fix from the original (which sorted the original vector, corrupting data for subsequent queries).

---

## Memory Layout

```
PlacementManager object on stack:
│
├── rounds_  [vector of 5 AVLTree objects]
│   ├── rounds_[0]  (ROUND_1)  → nodes: vector<Node> (contiguous in heap)
│   ├── rounds_[1]  (ROUND_2)  → nodes: vector<Node>
│   ├── rounds_[2]  (ROUND_3)  → nodes: vector<Node>
│   ├── rounds_[3]  (ROUND_4)  → nodes: vector<Node>
│   └── rounds_[4]  (FINAL_ROUND) → nodes: vector<Node>
│
├── studentInfo_  [unordered_map — heap]
│
├── stats_  [vector of 5 RoundStats — heap]
│   └── Each RoundStats has 6 unordered_maps
│
├── overall_  [OverallStats — inline, with heap-allocated maps/vectors]
│
└── 6 index sets  [each set<pair<...>> — heap, red-black tree internally]
```

All heap memory is managed through RAII containers (`std::vector`, `std::unordered_map`, `std::set`). No manual `new`/`delete`. Object lifetime == `PlacementManager p` lifetime in `main()`.

---

## Complexity Analysis

| Operation | Complexity | Notes |
|---|---|---|
| `loadCSV()` per record | O(log N) | AVL insert dominates |
| `filter()` on round r | O(K) | K = size of round r |
| `notSelected()` | O(N) | Scans R1, O(1) offer check |
| Existence check (single) | O(1) | `unordered_map::count()` |
| Existence check (composite) | O(log M) | `set::count()` on pair |
| `toVector()` | O(N) | In-order traversal |
| `computeMedian()` | O(K log K) | Sort of copy |
| `writeCSV()` | O(K) | K = result size |
| Memory (per company) | O(N × 5) | N records across 5 rounds |

> **N** = total student records loaded; **K** = size of filtered result; **M** = number of unique composite key combinations.
