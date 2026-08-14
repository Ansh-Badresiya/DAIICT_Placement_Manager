# Sequence Diagrams — DAIICT Placement Manager

> Sequence diagrams showing the exact call flow for all major modules: Data Loading, Sort/Display/Export, Placement Statistics, and Not-Selected Students.

---

## Table of Contents

- [Module 1: Data Input Flow](#module-1-data-input-flow)
- [Module 2: Sort / Display / Export Flow](#module-2-sort--display--export-flow)
  - [Sort Whole Data](#21-sort-whole-data)
  - [Sort with Single Filter (e.g., Batch Wise)](#22-sort-with-single-filter-eg-batch-wise)
  - [Sort with Composite Filter (e.g., Batch + Company)](#23-sort-with-composite-filter-eg-batch--company)
- [Module 3: Placement Statistics Flow](#module-3-placement-statistics-flow)
  - [Overall Statistics](#31-overall-statistics)
  - [Batch-Wise Statistics](#32-batch-wise-statistics)
  - [Student Placement Details](#33-student-placement-details)
- [Module 4: Not-Selected Students Flow](#module-4-not-selected-students-flow)
- [Module 5: AVL Tree Insert Flow](#module-5-avl-tree-insert-flow)

---

## Module 1: Data Input Flow

This flow is triggered when the user selects **Menu Option 1 → Input Placement Data**.

```mermaid
sequenceDiagram
    actor User
    participant main as main.cpp
    participant PM as PlacementManager
    participant AVL as AVLTree[Round]
    participant FS as File System (CSV)

    User->>main: Select Option 1 (Input Data)
    main->>PM: InputPlacementData()

    PM->>User: "Enter Company Name:"
    User->>PM: "Company1"

    loop For each round r = R1, R2, R3, R4, Final
        PM->>User: "Enter file path for Round r:"
        User->>PM: "data/Company1Rr.csv"
        PM->>PM: loadCSV(path, "Company1", r)

        PM->>FS: open(path)
        FS-->>PM: file handle

        PM->>FS: getline() — skip header
        FS-->>PM: header line (discarded)

        loop For each data row in CSV
            PM->>FS: getline()
            FS-->>PM: raw CSV line

            PM->>PM: parse fields (id, name, program, date→year, email, contactNO, whatsappNO)
            Note over PM: batch = id[0:4], year = date[6:10]

            alt r == FINAL_ROUND
                PM->>PM: parse package field
            end

            PM->>PM: rec.company = "Company1"
            PM->>PM: insertSorted(r, rec)
            PM->>AVL: insert(rec)
            AVL->>AVL: insertAt(root, rec) — O(log N)
            AVL->>AVL: rebalance via rotations if needed
            AVL-->>PM: updated root index

            PM->>PM: stats_[r].totalCount++
            PM->>PM: stats_[r].studentAttempts[id]++
            PM->>PM: stats_[r].studentCompanies[id].push_back(company)
            PM->>PM: stats_[r].batchAttempts[batch]++
            PM->>PM: stats_[r].companyAttempts[company]++
            PM->>PM: stats_[r].programAttempts[program]++
            PM->>PM: stats_[r].yearAttempts[year]++

            alt r == ROUND_1
                PM->>PM: batchCompanyIdx_.insert({batch, company})
                PM->>PM: batchProgramIdx_.insert({batch, program})
                PM->>PM: programCompanyIdx_.insert({program, company})
                PM->>PM: yearBatchIdx_.insert({year, batch})
                PM->>PM: yearProgramIdx_.insert({year, program})
                PM->>PM: yearCompanyIdx_.insert({year, company})
                PM->>PM: studentInfo_[id] = rec
            end

            alt r == FINAL_ROUND
                PM->>PM: overall_.totalOffers[id]++
                PM->>PM: overall_.offeredCompanies[id].push_back(company)
                PM->>PM: overall_.offeredPackages[id].push_back(package)
                PM->>PM: overall_.batchOffers[batch]++
                PM->>PM: overall_.programOffers[program]++
                PM->>PM: overall_.companyOffers[company]++
                PM->>PM: overall_.yearOffers[year]++
                PM->>PM: overall_.allPackages.push_back(package)
                PM->>PM: update min/max/avg package
            end
        end

        alt r == ROUND_1
            PM->>PM: overall_.companiesVisited.push_back(company)
        end

        PM->>FS: close file
        PM->>User: "Successfully fetched Round r data for Company1"
    end

    PM-->>main: return
    main-->>User: Show main menu again
```

---

## Module 2: Sort / Display / Export Flow

### 2.1 Sort Whole Data

Simplest sort — no filter criterion, just select round.

```mermaid
sequenceDiagram
    actor User
    participant main as main.cpp
    participant PM as PlacementManager
    participant AVL as AVLTree[Round]

    User->>main: Select Option 2 → Option 1 (Sort Whole Data)
    main->>PM: SortWholeData()

    PM->>PM: isDataLoaded()
    alt Data not loaded
        PM->>User: "No data loaded. Please insert data first."
    else Data loaded
        PM->>User: "Select Round (1=R1, 2=R2, ... 5=Final):"
        User->>PM: e.g., 2 (Round 2)
        PM->>PM: rd = ROUND_2, isFR = false

        PM->>AVL: toVector()
        AVL->>AVL: inorderAt(root, result) — O(N) in-order traversal
        AVL-->>PM: vector<StudentRecord> sorted by ID

        PM->>PM: askDisplayWrite(data, isFR=false, "All Data - Round 2")
        PM->>User: "Do you want to Display Data (Y/N)?"
        User->>PM: "Y"
        PM->>User: Prints formatted table (all columns, no package)

        PM->>User: "Enter File Path to store the data:"
        User->>PM: "output/round2_all.csv"
        PM->>PM: writeCSV("output/round2_all.csv", data, false)
        PM->>User: "Data Written Successfully"
    end
```

---

### 2.2 Sort with Single Filter (e.g., Batch Wise)

Pattern used by: `SortDataBatchWise`, `SortDataProgramWise`, `SortDataYearWise`, `SortDataCompanyWise`.

```mermaid
sequenceDiagram
    actor User
    participant main as main.cpp
    participant PM as PlacementManager
    participant AVL as AVLTree[Round]

    User->>main: Select Option 2 → Option 2 (Sort Batch Wise)
    main->>PM: SortDataBatchWise()

    PM->>PM: isDataLoaded()
    alt Data not loaded
        PM->>User: "No data loaded."
    else
        PM->>User: "Enter Batch:"
        User->>PM: 2021

        PM->>PM: batchExists(2021)
        Note over PM: O(1) — checks stats_[R1].batchAttempts.count(2021)
        alt Batch not found
            PM->>User: "Batch 2021 not found."
        else Batch found
            PM->>PM: askRound()
            PM->>User: "Select Round (1-5):"
            User->>PM: 3 (Round 3)
            PM->>PM: rd = ROUND_3, isFR = false

            PM->>PM: filter(ROUND_3, lambda: r.batch == 2021)
            PM->>AVL: filter(pred)
            AVL->>AVL: filterAt(root, pred, result) — in-order, only batch==2021
            AVL-->>PM: vector<StudentRecord> (batch 2021, sorted by ID)

            PM->>PM: askDisplayWrite(data, false, "Batch 2021 — Round 3")
            PM->>User: Display/Export prompt
        end
    end
```

---

### 2.3 Sort with Composite Filter (e.g., Batch + Company)

Pattern used by: `SortDataBatchAndCompanyWise`, `SortDataProgramOFBatchWise`, etc.

```mermaid
sequenceDiagram
    actor User
    participant PM as PlacementManager
    participant IDX as Index Sets
    participant AVL as AVLTree[Round]

    User->>PM: SortDataBatchAndCompanyWise()

    PM->>PM: isDataLoaded()
    PM->>User: "Enter Batch:"
    User->>PM: 2021
    PM->>User: "Enter Company Name:"
    User->>PM: "Company1"

    PM->>IDX: batchCompanyIdx_.count({2021, "Company1"})
    Note over IDX: O(log M) set lookup
    IDX-->>PM: found/not found

    alt Not found
        PM->>User: "Batch 2021 + Company 'Company1' not found."
    else Found
        PM->>PM: askRound()
        User->>PM: round selection

        PM->>PM: filter(rd, lambda: r.batch==2021 && r.company=="Company1")
        PM->>AVL: filter(composite predicate)
        AVL-->>PM: filtered vector

        PM->>PM: askDisplayWrite(data, isFR, label)
        PM->>User: Display/Export prompt
    end
```

---

## Module 3: Placement Statistics Flow

### 3.1 Overall Statistics

```mermaid
sequenceDiagram
    actor User
    participant PM as PlacementManager

    User->>PM: FindOverallPlacementStatistics()

    PM->>PM: isDataLoaded()

    PM->>User: Print horizontal line
    PM->>User: "# Overall Placement Statistics:"
    PM->>User: "No. Students in Round 1: " + stats_[R1].totalCount
    PM->>User: "No. Students in Round 2: " + stats_[R2].totalCount
    PM->>User: "No. Students in Round 3: " + stats_[R3].totalCount
    PM->>User: "No. Students in Round 4: " + stats_[R4].totalCount
    PM->>User: "No. Students Got Job Offer: " + stats_[FR].totalCount
    PM->>PM: success_rate = FR.totalCount / R1.totalCount * 100
    PM->>User: "Success Rate: " + success_rate + "%"

    PM->>User: "Max Package: " + overall_.maxPackage + " LPA"
    PM->>User: "Min Package: " + overall_.minPackage + " LPA"
    PM->>User: "Avg Package: " + overall_.avgPackage + " LPA"
    PM->>PM: computeMedian(overall_.allPackages)
    Note over PM: Sorts a COPY of allPackages — does not mutate original
    PM->>User: "Median Package: " + median + " LPA"

    PM->>User: "No. Of Companies Visited: " + overall_.companiesVisited.size()
    PM->>User: Print all company names (5 per line)
```

---

### 3.2 Batch-Wise Statistics

Pattern used by all statistics methods except Overall and Student Details.

```mermaid
sequenceDiagram
    actor User
    participant PM as PlacementManager
    participant AVL as AVLTree[Round]

    User->>PM: FindBatchWisePlacementStatistics()

    PM->>PM: isDataLoaded()
    PM->>User: "Enter Batch:"
    User->>PM: 2022

    PM->>PM: batchExists(2022)
    alt Not found
        PM->>User: "Batch not found."
    else Found
        Note over PM: Build predicate: r.batch == 2022
        PM->>AVL: filter(FINAL_ROUND, pred) → FR offers
        PM->>AVL: filter(ROUND_1, pred).size() → R1 attempts
        PM->>AVL: filter(ROUND_2, pred).size() → R2 attempts
        PM->>AVL: filter(ROUND_3, pred).size() → R3 attempts
        PM->>AVL: filter(ROUND_4, pred).size() → R4 attempts

        PM->>PM: statsBlock(frOffers, r1, r2, r3, r4, "Batch 2022 Statistics", medFn)
        PM->>PM: computeMedian(packages from FR offers)
        PM->>User: Print full statistics block:
        Note over PM,User: No. Students R1/R2/R3/R4, Offers, Success Rate,\nMax/Min/Avg/Median Package, Unique Program-Company combos
    end
```

---

### 3.3 Student Placement Details

```mermaid
sequenceDiagram
    actor User
    participant PM as PlacementManager
    participant SI as studentInfo_ map
    participant RS as RoundStats[5]
    participant OS as OverallStats

    User->>PM: FindStudentPlacementDetails()

    PM->>PM: isDataLoaded()
    PM->>User: "Enter Student's ID:"
    User->>PM: 202101126

    PM->>SI: studentInfo_.count(202101126)
    SI-->>PM: found/not found

    alt Not found
        PM->>User: "Invalid ID. Student not found."
    else Found
        PM->>SI: studentInfo_.at(202101126)
        SI-->>PM: StudentRecord (name, batch, program, email, contactNO, whatsappNO)

        PM->>User: Print student info block (name, ID, batch, program, email, contacts)

        PM->>OS: totalOffers.count(202101126)
        OS-->>PM: offers count
        PM->>RS: stats_[R1].studentAttempts[202101126]
        RS-->>PM: R1 attempt count

        PM->>PM: success_rate = offers / R1attempts * 100
        PM->>User: "Success Rate: " + success_rate + "%"

        loop For r = R1, R2, R3, R4
            PM->>RS: stats_[r].studentAttempts[202101126]
            RS-->>PM: attempt count for this round
            PM->>RS: stats_[r].studentCompanies[202101126]
            RS-->>PM: list of companies for this round
            PM->>User: Print round attempts + companies
        end

        PM->>OS: offeredCompanies[202101126]
        OS-->>PM: companies that made offer
        PM->>OS: offeredPackages[202101126]
        OS-->>PM: packages offered

        PM->>User: Print "No. of Job Offers: N, Companies: ..., Packages: ..."
    end
```

---

## Module 4: Not-Selected Students Flow

Pattern shared by all 10 `FindNotSelectedXxx()` methods.

```mermaid
sequenceDiagram
    actor User
    participant PM as PlacementManager
    participant AVL as AVLTree[ROUND_1]
    participant OS as OverallStats

    User->>PM: FindNotSelectedBatchWise()

    PM->>PM: isDataLoaded()
    PM->>User: "Enter Batch:"
    User->>PM: 2021

    PM->>PM: batchExists(2021)
    alt Not found
        PM->>User: "Batch not found."
    else Found
        PM->>PM: notSelected(lambda: r.batch == 2021)

        PM->>AVL: filter(composite_lambda)
        Note over AVL: Composite lambda = (r.batch == 2021) AND (overall_.totalOffers.count(r.id) == 0)

        loop For each node in AVL (in-order traversal)
            AVL->>OS: totalOffers.count(student.id)
            OS-->>AVL: 0 (not selected) or N (has offers)
            alt totalOffers == 0 AND batch == 2021
                AVL->>AVL: add to result
            end
        end

        AVL-->>PM: vector<StudentRecord> (not-selected students, batch 2021)

        alt result is empty
            PM->>User: "Wow!!! All students of Batch 2021 are placed!"
        else result not empty
            PM->>PM: askDisplayWrite(data, false, "Not Selected — Batch 2021")
            PM->>User: Display/Export prompt
        end
    end
```

---

## Module 5: AVL Tree Insert Flow

Detailed view of what happens when `insertSorted()` is called.

```mermaid
sequenceDiagram
    participant PM as PlacementManager
    participant AVL as AVLTree

    PM->>AVL: insert(studentRecord)
    AVL->>AVL: root = insertAt(root, data)

    loop Recursive insertAt(i, data)
        alt i == -1 (empty slot)
            AVL->>AVL: nodes.push_back(Node(data))
            AVL-->>AVL: return new node index
        else data < nodes[i].data
            AVL->>AVL: newLeft = insertAt(nodes[i].left, data)
            AVL->>AVL: nodes[i].left = newLeft
        else nodes[i].data < data
            AVL->>AVL: newRight = insertAt(nodes[i].right, data)
            AVL->>AVL: nodes[i].right = newRight
        else duplicate key
            AVL->>AVL: nodes[i].data = data (overwrite)
            AVL-->>AVL: return i (no rebalance needed)
        end

        AVL->>AVL: updateHeight(i)
        AVL->>AVL: bf = getBalance(i)

        alt bf > 1 AND data < left child's data
            Note over AVL: Left-Left case
            AVL->>AVL: return rotateRight(i)
        else bf < -1 AND data > right child's data
            Note over AVL: Right-Right case
            AVL->>AVL: return rotateLeft(i)
        else bf > 1 AND data > left child's data
            Note over AVL: Left-Right case
            AVL->>AVL: nodes[i].left = rotateLeft(nodes[i].left)
            AVL->>AVL: return rotateRight(i)
        else bf < -1 AND data < right child's data
            Note over AVL: Right-Left case
            AVL->>AVL: nodes[i].right = rotateRight(nodes[i].right)
            AVL->>AVL: return rotateLeft(i)
        else balanced
            AVL-->>AVL: return i
        end
    end

    AVL-->>PM: tree updated (root may have changed)
```

---

## Summary: Method Call Patterns

All 32 public methods share one of **4 common call patterns**:

### Pattern A — Sort/Export with No Filter
```
SortWholeData()
  └── isDataLoaded() → askRound() → toVector() → askDisplayWrite()
```

### Pattern B — Sort/Export with Single Filter
```
SortDataBatchWise() / SortDataProgramWise() / SortDataYearWise() / SortDataCompanyWise()
  └── isDataLoaded() → get criterion → validateXxx() → askRound() → filter(lambda) → askDisplayWrite()
```

### Pattern C — Sort/Export with Composite Filter
```
SortDataBatchAndCompanyWise() / SortDataProgramOFBatchWise() / ...
  └── isDataLoaded() → get 2 criteria → validateComposite() → askRound() → filter(composite lambda) → askDisplayWrite()
```

### Pattern D — Statistics
```
FindXxxWisePlacementStatistics()
  └── isDataLoaded() → get criterion → validate() → filter(FR, pred) + filter(R1..R4, pred).size() → statsBlock()
```

### Pattern E — Not Selected
```
FindNotSelectedXxx()
  └── isDataLoaded() → get criterion → validate() → notSelected(lambda) → askDisplayWrite() or "All placed!" message
```
