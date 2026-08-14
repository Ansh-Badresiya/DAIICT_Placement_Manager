# DAIICT Placement Manager

> A high-performance, console-based **C++ Placement Management System** designed for managing multi-round campus recruitment data for Dhirubhai Ambani Institute of Information and Communication Technology (DA-IICT).

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Project Structure](#project-structure)
- [Architecture Summary](#architecture-summary)
- [Data Model](#data-model)
- [Quick Start](#quick-start)
- [Menu Navigation](#menu-navigation)
- [Documentation Index](#documentation-index)
- [Design Philosophy & Optimizations](#design-philosophy--optimizations)
- [Technologies Used](#technologies-used)

---

## Overview

The **DAU Placement Manager (Optimized)** is a refactored, production-quality C++ console application that tracks, analyzes, and exports campus placement data for multiple companies across multiple interview rounds. The system handles:

- Loading student records from CSV files for **5 placement rounds** (Round 1 → Round 4 → Final Round)
- **Sorted in-memory storage** using a self-balancing AVL Tree
- Rich **placement statistics** across multiple dimensions (batch, program, year, company)
- **Not-Selected student tracking** to identify students who didn't receive any offers
- Data **display and CSV export** for every query result

This is an **optimized rewrite** of an earlier version — the internal data structures and algorithms have been completely redesigned for correctness and performance, while keeping the user-facing menu interface identical.

---

## Key Features

| Feature | Description |
|---|---|
| **AVL Tree Storage** | O(log N) insert, cache-friendly in-order traversal via array-indexed nodes |
| **O(1) Existence Checks** | `std::set`-based index sets for batch/program/company/year lookups |
| **32 Query Operations** | Sort (10) + Statistics (12) + Not-Selected (10) across multiple filter dimensions |
| **CSV Export** | Every query result can be saved to a user-specified CSV file |
| **Generic Filter Engine** | Single `filter()` function replaces ~80 duplicated filter helpers from original |
| **Bug-Fixed Median** | `computeMedian()` takes by value — fixes data mutation bug in original |
| **RAII Memory Safety** | No raw `new`/`delete` — all memory managed via `std::vector` |
| **Proper Header Split** | Clean `include/` + `src/` separation, no `#include`-ing of `.cpp` files |

---

## Project Structure

```
Placement_Manager_Optimized/
│
├── main.cpp                    # Entry point — menu loop & sub-menus
│
├── include/
│   ├── StudentRecord.h         # Core data model (StudentRecord, RoundStats, OverallStats, Round enum)
│   ├── AVLTree.h               # Template AVL Tree (array-indexed, cache-friendly)
│   └── PlacementManager.h      # Class declaration — all public & private members
│
├── src/
│   └── PlacementManager.cpp    # Full implementation (~1428 lines)
│
├── data/
│   ├── Company1R1.csv          # Round 1 data for Company 1
│   ├── Company1R2.csv          # Round 2 data for Company 1
│   ├── Company1R3.csv          # Round 3 data for Company 1
│   ├── Company1R4.csv          # Round 4 data for Company 1
│   ├── Company1FR.csv          # Final Round data for Company 1 (includes package)
│   └── ... (5 companies × 5 rounds = 25 CSV files)
│
├── docs/                       # Project documentation (you are here)
│   ├── README.md               # This file — project overview
│   ├── ARCHITECTURE.md         # Deep-dive into system architecture
│   ├── CLASS_DIAGRAM.md        # UML class diagram (Mermaid)
│   ├── SEQUENCE_DIAGRAMS.md    # Sequence diagrams for all major flows
│   ├── DATA_FLOW.md            # Data flow & CSV format specification
│   ├── API_REFERENCE.md        # Complete public API documentation
│   └── DESIGN_DECISIONS.md     # Rationale for key design choices
│
├── README.md                   # Root-level readme (brief)
└── main.exe                    # Compiled binary
```

---

## Architecture Summary

```
┌──────────────────────────────────────────────────────┐
│                      main.cpp                        │
│   (Menu Loop → Sub-Menus → PlacementManager calls)   │
└─────────────────────┬────────────────────────────────┘
                      │ uses
                      ▼
┌──────────────────────────────────────────────────────┐
│                 PlacementManager                     │
│                                                      │
│  Storage:                                            │
│    rounds_[5]   → AVLTree<StudentRecord> per round   │
│    studentInfo_ → unordered_map<id, StudentRecord>   │
│    stats_[5]    → RoundStats per round               │
│    overall_     → OverallStats (cross-round)         │
│    Index Sets   → O(1) existence check (6 sets)      │
│                                                      │
│  Core Operations:                                    │
│    loadCSV()    → parse CSV → insertSorted()         │
│    filter()     → AVLTree::filter(lambda)            │
│    notSelected()→ R1 filter + offer check            │
│    askDisplayWrite() → printTable() + writeCSV()     │
└──────────────┬─────────────────────┬─────────────────┘
               │                     │
               ▼                     ▼
┌─────────────────────┐   ┌───────────────────────────┐
│  AVLTree<T>         │   │  StudentRecord / structs  │
│  (array-indexed)    │   │  RoundStats, OverallStats │
│  insert() O(log N)  │   │  Round enum               │
│  toVector() O(N)    │   └───────────────────────────┘
│  filter()   O(N)    │
└─────────────────────┘
```

---

## Data Model

### Student Record Fields

| Field | Type | Description | Example |
|---|---|---|---|
| `id` | `long long` | Unique student ID (YYYYNNNNN format) | `202101126` |
| `name` | `string` | Full name | `Dhanuk Garde` |
| `batch` | `int` | Admission year (first 4 digits of ID) | `2021` |
| `program` | `string` | Degree program | `Btech ICT-CS` |
| `email` | `string` | Institute email | `202101126@daiict.ac.in` |
| `contactNO` | `long long` | Contact number | `2859914207` |
| `whatsappNO` | `long long` | WhatsApp number | `8423114879` |
| `company` | `string` | Company name | `Company1` |
| `year` | `int` | Interview year (parsed from date column) | `2023` |
| `package` | `float` | Salary package in LPA (Final Round only) | `15.5` |

### Programs Tracked
- **Btech ICT** — Bachelor of Technology in ICT
- **Btech ICT-CS** — Bachelor of Technology in ICT with CS specialization
- **Btech MnC** — Bachelor of Technology in Mathematics and Computing
- **Mtech ICT** — Master of Technology in ICT
- **Mtech ICT-CS** — Master of Technology in ICT-CS
- **Mtech MnC** — Master of Technology in Mathematics and Computing

---

## Quick Start

### Prerequisites
- C++17 compatible compiler (g++, MSVC, clang)
- CSV data files in `data/` directory

### Compile & Run

```bash
# Compile
g++ -std=c++17 -O2 -I include -o main.exe main.cpp src/PlacementManager.cpp

# Run
./main.exe          # Windows (PowerShell), Linux, and macOS
```

### Loading Data (Example)

```
> Enter Company's Name: TechCorp
> Enter file path for Round 1: data/Company1R1.csv
> Enter file path for Round 2: data/Company1R2.csv
> Enter file path for Round 3: data/Company1R3.csv
> Enter file path for Round 4: data/Company1R4.csv
> Enter file path for Final Round: data/Company1FR.csv
```

> **Note:** Data for **all 5 rounds** must be loaded for one company before querying.

---

## Menu Navigation

```
Main Menu
├── 1. Input Placement Data       → Load 5 CSVs for a company
├── 2. Sort Data                  → View/export filtered sorted records
│   ├── 1. Whole Data
│   ├── 2. Batch Wise
│   ├── 3. Program Wise
│   ├── 4. Year Wise
│   ├── 5. Company Wise
│   ├── 6. Batch + Company Wise
│   ├── 7. Program of Batch Wise
│   ├── 8. Program of Company Wise
│   ├── 9. Program of Year Wise
│   └── 10. Year + Batch Wise
├── 3. View Placement Statistics  → Aggregated stats with package info
│   ├── 1. Overall Statistics
│   ├── 2. Student Placement Details (by ID)
│   ├── 3. Batch Wise
│   ├── 4. Program Wise
│   ├── 5. Company Wise
│   ├── 6. Year Wise
│   ├── 7. Batch + Company Wise
│   ├── 8. Program + Batch Wise
│   ├── 9. Program + Company Wise
│   ├── 10. Year + Batch Wise
│   ├── 11. Year + Company Wise
│   └── 12. Year + Program Wise
├── 4. View Not Selected Students → Students with 0 job offers
│   ├── 1. Batch Wise
│   ├── 2. Program Wise
│   ├── 3. Company Wise
│   ├── 4. Year Wise
│   ├── 5. Batch + Program Wise
│   ├── 6. Batch + Company Wise
│   ├── 7. Company + Program Wise
│   ├── 8. Year + Batch Wise
│   ├── 9. Year + Program Wise
│   └── 10. Year + Company Wise
└── 5. Exit
```

---

## Documentation Index

| Document | Description |
|---|---|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Detailed system architecture, class responsibilities, memory layout |
| [CLASS_DIAGRAM.md](./CLASS_DIAGRAM.md) | UML class diagram showing all classes, attributes, and relationships |
| [SEQUENCE_DIAGRAMS.md](./SEQUENCE_DIAGRAMS.md) | Sequence diagrams for Data Loading, Sort/Filter, Statistics, Not-Selected flows |
| [DATA_FLOW.md](./DATA_FLOW.md) | CSV format spec, data ingestion pipeline, derived fields |
| [API_REFERENCE.md](./API_REFERENCE.md) | Full API reference for all public and private methods |
| [DESIGN_DECISIONS.md](./DESIGN_DECISIONS.md) | Rationale behind AVL Tree, index sets, single filter(), and other key choices |

---

## Design Philosophy & Optimizations

### vs. Original Version

| Aspect | Original | Optimized |
|---|---|---|
| **Data Storage** | Multiple `vector`s + linked lists | `AVLTree<StudentRecord>` per round |
| **Insert Complexity** | O(N) worst case | O(log N) guaranteed |
| **Existence Check** | O(N) linear scan | O(1) via `std::set` index |
| **Filter Functions** | ~80 duplicated helpers | 1 generic `filter(Round, lambda)` |
| **Table Printers** | ~40 duplicated functions | 1 `printTable()` + 1 `printTableRow()` |
| **CSV Writers** | ~40 duplicated functions | 1 `writeCSV()` |
| **Median Bug** | Mutates caller's data | Fixed — takes by value |
| **Memory Safety** | Manual `new`/`delete` | RAII via `std::vector` |
| **Build System** | `#include` of `.cpp` | Proper header/source split |

---

## Technologies Used

- **Language:** C++17
- **Standard Library:** `<vector>`, `<unordered_map>`, `<set>`, `<functional>`, `<algorithm>`, `<fstream>`, `<sstream>`, `<iomanip>`
- **Data Structure:** Custom template AVL Tree (array-indexed, no heap allocation per node)
- **Build:** Any C++17 compliant compiler (g++, clang++, MSVC)
- **Data Format:** CSV (Comma Separated Values)

---

*Generated documentation for the DAU Placement Manager — Optimized project.*
