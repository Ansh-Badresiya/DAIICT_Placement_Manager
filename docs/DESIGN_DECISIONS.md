# Design Decisions — DAIICT Placement Manager

> This document explains the rationale behind every significant design choice in the optimized rewrite. Understanding *why* decisions were made is as important as understanding *what* was built.

---

## Table of Contents

1. [AVL Tree Instead of std::vector](#1-avl-tree-instead-of-stdvector)
2. [Array-Indexed Nodes (No Pointer-per-Node)](#2-array-indexed-nodes-no-pointer-per-node)
3. [One AVL Tree per Round](#3-one-avl-tree-per-round)
4. [Single Generic filter() Function](#4-single-generic-filter-function)
5. [Index Sets for O(1) Composite Existence Checks](#5-index-sets-for-o1-composite-existence-checks)
6. [computeMedian() Takes by Value](#6-computemedian-takes-by-value)
7. [Proper Header/Source Split](#7-proper-headersource-split)
8. [RAII via std::vector (No Manual Memory)](#8-raii-via-stdvector-no-manual-memory)
9. [RoundStats and OverallStats Separation](#9-roundstats-and-overallstats-separation)
10. [studentInfo_ Map for Direct ID Lookup](#10-studentinfo_-map-for-direct-id-lookup)
11. [Lambda Predicates Instead of Specialized Functions](#11-lambda-predicates-instead-of-specialized-functions)
12. [Single printTable() and writeCSV()](#12-single-printtable-and-writecsv)
13. [Batch and Year Derived from Existing Fields](#13-batch-and-year-derived-from-existing-fields)
14. [SORT_WITH_FILTER Macro](#14-sort_with_filter-macro)

---

## 1. AVL Tree Instead of std::vector

**Decision:** Use a custom `AVLTree<StudentRecord>` for per-round storage instead of `std::vector`.

**Rationale:**

The original version used `std::vector` with linear-scan insertions (O(N) worst case). As the number of students per company grows, this causes quadratic time for bulk loading.

An AVL tree guarantees **O(log N) insert** regardless of data distribution — even if student IDs arrive in sorted or reverse-sorted order (which would cause degenerate behavior in a BST).

Additionally, AVL trees provide **O(N) sorted traversal for free** — `toVector()` and `filter()` just do an in-order walk, always returning results sorted by student ID. With a plain vector, you'd need a separate `std::sort()` pass every time (O(N log N)).

**Trade-off accepted:** Slightly higher constant factor per operation compared to `push_back()` due to height updates and potential rotations. However, this is amortized out over bulk loading and is far outweighed by the algorithmic improvement.

---

## 2. Array-Indexed Nodes (No Pointer-per-Node)

**Decision:** Store all AVL tree nodes in a `vector<Node>` and use `int` indices instead of `Node*` pointers.

**Rationale:**

Traditional AVL tree implementations use `new Node(...)` per insertion — one heap allocation per record. For 500+ records across 5 rounds × 5 companies, that's 12,500+ small heap allocations, causing:
- Memory fragmentation
- Poor cache locality (each `Node*` dereference may be a cache miss)
- Overhead from the allocator itself

Using a `vector<Node>`:
- All nodes live in a **contiguous block** of memory
- In-order traversal visits physically nearby memory → **cache-friendly**
- Single heap allocation for the entire vector (amortized growth)
- No per-node destructor or memory leak risk

The `int` index (`-1` for null) replaces a pointer. This is safe because indices are stable after reservation, and we never remove nodes (only insert).

> **Note:** After `push_back()`, references and pointers into a `vector` may be invalidated (if reallocation occurs). This is why `insertAt()` stores the return value of recursive calls **before** using indices from `nodes[i]`:
> ```cpp
> int newLeft = insertAt(nodes[i].left, data);
> nodes[i].left = newLeft;  // Safe — nodes[i] re-read after potential reallocation
> ```

---

## 3. One AVL Tree per Round

**Decision:** Maintain 5 separate AVL trees, one per round (`rounds_[ROUND_1]` through `rounds_[FINAL_ROUND]`).

**Rationale:**

Almost every query is **round-specific** — "sort batch 2021's data for Round 2", "find statistics in Final Round", etc. If all rounds shared a single tree, every filter would need an additional `rec.round == r` predicate, visiting more nodes unnecessarily.

With one tree per round:
- `filter(ROUND_2, pred)` only visits R2 nodes
- Round-specific tree sizes are smaller → faster traversal
- `toVector()` for a specific round returns only that round's records

The 5-tree approach also naturally separates data concerns: R1 is the universe of participants; FR is the universe of offer recipients.

---

## 4. Single Generic filter() Function

**Decision:** Replace ~80 specialized filter helper functions with a single:
```cpp
vector<StudentRecord> filter(Round r, function<bool(const StudentRecord&)> pred) const;
```

**Rationale:**

The original had duplicated filter logic for every combination of (round × filter criterion). The body of each was essentially identical: walk the data, apply a condition, collect matches. This is a classic **Strategy pattern** opportunity.

By accepting a `std::function<bool(const StudentRecord&)>` predicate, the same traversal logic handles:
- Single-field filters: `[&](const StudentRecord& r) { return r.batch == batch; }`
- Composite filters: `[&](const StudentRecord& r) { return r.batch == batch && r.company == comp; }`
- Not-selected filters (combines with offer check)

This eliminates **~80 functions** in the original, reducing the codebase by ~1000+ lines while making the logic easier to verify (one function to audit, not 80).

**Trade-off:** Slight overhead from `std::function` (potential virtual dispatch/type-erasure). This is negligible at the scale of this application (hundreds of records, not millions).

---

## 5. Index Sets for O(1) Composite Existence Checks

**Decision:** Use six `std::set<pair<...>>` index structures for composite existence validation.

```cpp
set<pair<int,    string>> batchCompanyIdx_;
set<pair<int,    string>> batchProgramIdx_;
set<pair<string, string>> programCompanyIdx_;
set<pair<int,    int>>    yearBatchIdx_;
set<pair<int,    string>> yearProgramIdx_;
set<pair<int,    string>> yearCompanyIdx_;
```

**Rationale:**

Before displaying filtered data, the system must validate that the requested combination actually exists in the dataset (e.g., "does batch 2021 have students at Company1?"). The original did this by scanning vectors — O(N) per check.

With a `std::set`, `count({batch, company})` runs in O(log M) (red-black tree lookup) where M is the number of unique combinations — far smaller than N (total records).

**Why `std::set` and not `std::unordered_set`?**
- `std::pair` doesn't have a built-in hash, so using `unordered_set` would require a custom hash
- `std::set` works out of the box with `std::pair`'s `operator<`
- O(log M) is perfectly acceptable for these validation checks

**Why populated only from R1 data?**
- Round 1 is the largest set (most students appear here)
- If a combination exists in any round, it will appear in R1
- This keeps index set sizes manageable

---

## 6. computeMedian() Takes by Value

**Decision:**
```cpp
float computeMedian(vector<float> nums) const;  // by value
```

**Rationale (Bug Fix):**

The original `computeMedian()` took `vector<float>&` (by reference) and called `sort()` on it. This **modified the caller's data** — specifically, it sorted `overall_.allPackages` in place, corrupting the vector for any subsequent call to `FindOverallPlacementStatistics()`.

Taking by value creates a local copy. The copy is sorted for median computation. The original data is untouched.

This is a **correctness bug** in the original, not just a style issue. The fix is essential.

**Performance note:** The copy costs O(K) memory and time for a vector of K packages. Since this is only called on result sets (not the full dataset), the cost is acceptable. The alternative — using `std::nth_element` on a copy — would be O(K) time but still requires a copy for correctness.

---

## 7. Proper Header/Source Split

**Decision:** Separate `include/PlacementManager.h` (declaration) and `src/PlacementManager.cpp` (definition). Do not `#include` any `.cpp` file.

**Rationale:**

The original project used `#include "PlacementManager.cpp"` directly in `main.cpp` — a practice that:
- Violates the One Definition Rule (ODR) if ever compiled with multiple translation units
- Prevents incremental compilation (every change recompiles everything)
- Makes the codebase harder to scale

The optimized version properly separates declaration from definition. The compiler sees each `.cpp` file as a separate **translation unit**, compiled independently and linked together. This is how professional C++ projects are structured.

**File responsibilities:**
| File | Contains |
|---|---|
| `include/StudentRecord.h` | Structs, enums, inline helper |
| `include/AVLTree.h` | Full template implementation (must be in header) |
| `include/PlacementManager.h` | Class declaration, `#include`s |
| `src/PlacementManager.cpp` | All method definitions |
| `main.cpp` | Entry point, menu functions |

**AVL Tree in header:** Template class implementations must be in headers (or explicitly instantiated in `.cpp`). This is a C++ language requirement, not a design decision.

---

## 8. RAII via std::vector (No Manual Memory)

**Decision:** Use `std::vector<Node>` inside `AVLTree` instead of raw `Node*` pointers.

**Rationale:**

The original code used manual `new`/`delete` for linked list/tree nodes. This is error-prone:
- Forgetting `delete` → memory leak
- Double-delete → undefined behavior
- Exception-unsafe (destructor may not run)

`std::vector` is a RAII container — when the `AVLTree` object goes out of scope, `~vector()` is automatically called, freeing all node memory. Zero destructor code needed.

The same principle applies throughout:
- `RoundStats` uses `unordered_map` (RAII)
- `OverallStats` uses `vector<float>`, `unordered_map` (RAII)
- `PlacementManager` uses `vector<AVLTree<...>>` (RAII)

The entire program has **zero manual memory management**.

---

## 9. RoundStats and OverallStats Separation

**Decision:** Two separate aggregate structures — `RoundStats` (per-round) and `OverallStats` (cross-round, offer-focused).

**Rationale:**

Different queries need different aggregation:
- "How many students were in Round 2?" → needs per-round counts → `RoundStats`
- "How many offers did student X receive?" → cross-round → `OverallStats`
- "What is the overall average package?" → offer-only data → `OverallStats`

Merging these into one structure would waste memory (most fields irrelevant for non-FR rounds) and make queries more complex.

The separation also makes it clear that `OverallStats` is **only updated during Final Round** loading, while `RoundStats` is updated for every round.

**Original:** Had ~25 separate `unordered_map` member variables. The restructured version collapses them into 2 structs with clear responsibilities.

---

## 10. studentInfo_ Map for Direct ID Lookup

**Decision:** Maintain a separate `unordered_map<long long, StudentRecord> studentInfo_` populated from R1 data.

**Rationale:**

`FindStudentPlacementDetails()` needs to look up a student by ID in O(1). Without this map, we'd have to do an O(N) scan of one of the AVL trees (or an O(log N) search, but AVL trees in this design use in-order traversal, not key-based lookup by ID directly without adding a `find()` method).

`studentInfo_` is populated from R1 because:
- Round 1 contains the most students (everyone who participated in any round participated in R1)
- Student personal info (name, program, email, contacts) doesn't change across rounds

**Memory trade-off:** This stores one complete `StudentRecord` per student in an extra map. For N students, this is O(N) extra memory — considered acceptable for O(1) lookup.

---

## 11. Lambda Predicates Instead of Specialized Functions

**Decision:** Use C++ lambda closures as predicates, capturing filter criteria from local variables.

```cpp
// From SortDataBatchWise():
auto data = filter(rd, [&](const StudentRecord& r) { return r.batch == batch; });
```

**Rationale:**

Lambda closures are:
- **Concise:** The predicate is defined at the point of use, immediately clear what it filters
- **Safe:** Captures by reference `[&]` — no need to copy `batch`, `company`, etc. into a separate struct
- **Composable:** Multiple conditions are naturally expressed: `r.batch == batch && r.company == comp`
- **Type-erased by std::function:** Compatible with the generic `filter()` signature

The alternative (template function with a comparator struct) would require a separate struct definition per filter variant — more boilerplate, less readable.

---

## 12. Single printTable() and writeCSV()

**Decision:** One `printTable()` function with a `bool showPackage` flag, replacing ~40 specialized table printers in the original.

**Rationale:**

The only structural difference between tables for different rounds/filters is:
- Whether the `Package` column is shown (only for Final Round or when explicitly requested)

Rather than duplicating the entire table-rendering logic 40 times, a single boolean flag handles the distinction:

```cpp
void printTable(const vector<StudentRecord>& data, bool showPackage) const;
```

The same applies to `writeCSV()`. This is the **DRY principle** (Don't Repeat Yourself) applied aggressively — verified correct in one place, impossible to have inconsistent behavior between variants.

---

## 13. Batch and Year Derived from Existing Fields

**Decision:** Derive `batch` from the first 4 digits of `Student ID`, and `year` from the interview date string.

**Rationale:**

The CSV does not have explicit `batch` or `year` columns. Instead of storing the raw date string and parsing it on every query, these values are **eagerly derived at load time** and stored in each `StudentRecord`:

```cpp
rec.batch = stoi(id_str.substr(0, 4));    // "202101126" → 2021
rec.year  = stoi(date_str.substr(6, 4));  // "01-04-2023" → 2023
```

This is a classic **computation vs. storage trade-off** — we pay a small constant cost at load time to make all queries cheaper (no need to re-parse strings during filtering).

---

## 14. SORT_WITH_FILTER Macro

**Decision:** Use a preprocessor macro `SORT_WITH_FILTER(predicate, header_str, is_fr)` to express the repeated round-selection + filter + display pattern.

```cpp
#define SORT_WITH_FILTER(predicate, header_str, is_fr)      \
    do {                                                    \
        Round rd = askRound();                              \
        bool isFR = (rd == FINAL_ROUND);                    \
        auto data = filter(rd, predicate);                  \
        askDisplayWrite(data, isFR || (is_fr), header_str); \
    } while (0)
```

**Rationale:**

The pattern `askRound() → filter() → askDisplayWrite()` appears verbatim in nearly every Sort method. Rather than copy-pasting 4 lines 10 times (with potential for divergence), the macro encapsulates the pattern.

The `do { ... } while(0)` wrapper ensures the macro behaves as a single statement in all syntactic contexts (no dangling `else` problems).

**Alternative considered:** A private helper method with callback. This was rejected because `auto data = filter(rd, predicate)` requires the predicate to be in scope as a lambda — not easily passed as a parameter type-safely without `std::function` overhead for a simple pattern.

---

## Summary: Improvements Over Original

| Issue in Original | Fix in Optimized |
|---|---|
| O(N) vector insert | O(log N) AVL Tree insert |
| O(N) existence checks | O(1) / O(log M) index sets |
| ~80 filter functions | 1 generic `filter()` |
| ~40 table printers | 1 `printTable()` |
| ~40 CSV writers | 1 `writeCSV()` |
| Median mutates data | `computeMedian()` takes by value |
| `#include "PlacementManager.cpp"` | Proper header/source split |
| Manual `new`/`delete` | RAII via `std::vector` |
| ~25 separate maps | 2 structured aggregates (RoundStats, OverallStats) |
| Per-node heap allocation (tree) | Contiguous `vector<Node>` (cache-friendly) |
