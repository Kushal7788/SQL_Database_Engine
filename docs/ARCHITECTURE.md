# Architecture

This document describes how the engine is put together: the components and their responsibilities, how a single command travels through them, the main types, the on-disk layout, and the algorithms behind the join, aggregation and matrix operators. Everything here is derived from the sources in `src/`.

## 1. Components

```mermaid
flowchart TB
    subgraph Frontend
        S["server.cpp (REPL + tokenizer)"]
        SP["syntacticParser + executors/*: syntacticParse*"]
        SM["semanticParser + executors/*: semanticParse*"]
        EX["executor.cpp (dispatch)"]
    end
    subgraph Operators["executors/"]
        REL["LOAD, PRINT, LIST, RENAME, EXPORT, CLEAR"]
        RA["SELECT, PROJECT, CROSS"]
        JN["JOIN USING NESTED / PARTHASH"]
        GB["GROUP BY"]
        MX["LOAD/PRINT/EXPORT MATRIX, CROSS_TRANSPOSE"]
    end
    subgraph Catalogues
        TC["TableCatalogue"]
        MC["MatrixCatalogue"]
    end
    subgraph Storage
        T["Table"]
        M["Matrix"]
        C["Cursor"]
        BM["BufferManager (FIFO pool)"]
        P["Page"]
    end
    DISK[("data/*.csv and data/temp/*")]
    LOG[("src/log")]

    S --> SP --> SM --> EX
    SM --> TC
    SM --> MC
    EX --> REL
    EX --> RA
    EX --> JN
    EX --> GB
    EX --> MX
    REL --> TC
    RA --> TC
    JN --> TC
    GB --> TC
    MX --> MC
    TC --> T
    MC --> M
    T --> C
    C --> BM
    BM --> P
    P --> DISK
    T --> DISK
    M --> DISK
    S -.-> LOG
```

*Figure 1. Component diagram. The front end turns a text line into a validated `ParsedQuery`; operators resolve relations through the catalogues and access data through cursors, the buffer manager and pages. Matrices bypass the page/cursor layer and manage their own tile files.*

The front end is three stages wired together by global state declared in `global.h`: `tokenizedQuery` (the token vector), `parsedQuery` (a single `ParsedQuery` instance that is cleared before each command), and the two catalogues. Every command lives in its own file under `src/executors/` (plus `src/clear.cpp`) and contributes three functions: `syntacticParseX`, `semanticParseX` and `executeX`. Adding a command means adding a `QueryType` value, one dispatch line in each of the three dispatchers, and that file.

The storage layer has a strict layering. Operators read rows only through a `Cursor`; a cursor obtains pages only from the `BufferManager`; only `Page` reads page files. Writes go the other way: operators append rows to a result CSV with `Table::writeRow`, and `Table::blockify` turns that CSV into page files via `BufferManager::writePage`. A `Logger` writes a trace line for nearly every function call to `./log`.

Global tuning constants are defined in `server.cpp`:

| Constant | Value | Meaning |
| --- | --- | --- |
| `BLOCK_SIZE` | `8` | Page size in KB (rows per page = `floor(BLOCK_SIZE * 1000 / (4 * columns))`) |
| `BLOCK_COUNT` | `2` | Pages held by the buffer pool |
| `PRINT_COUNT` | `20` | Maximum rows shown by `PRINT` |

## 2. Lifecycle of a command

```mermaid
sequenceDiagram
    participant U as User
    participant S as server.cpp
    participant SY as syntacticParse
    participant SE as semanticParse
    participant TC as TableCatalogue
    participant EX as executeSELECTION
    participant CU as Cursor
    participant BM as BufferManager
    participant RT as Result Table

    U->>S: "R <- SELECT Salary >= 30000 FROM EMPLOYEE"
    S->>S: regex split into tokens
    S->>SY: syntacticParse()
    SY->>SY: tokens[2] is SELECT, fill ParsedQuery
    SY-->>S: true
    S->>SE: semanticParse()
    SE->>TC: isTable(R)? isTable(EMPLOYEE)? isColumnFromTable(Salary)?
    TC-->>SE: R absent, EMPLOYEE and Salary present
    SE-->>S: true
    S->>EX: executeCommand()
    EX->>RT: new Table(R, columns) writes header to data/temp/R.csv
    EX->>CU: EMPLOYEE.getCursor()
    loop every row
        CU->>BM: getPage(EMPLOYEE, k)
        BM-->>CU: Page from pool or disk
        CU-->>EX: row
        EX->>RT: writeRow(row) if predicate holds
    end
    EX->>RT: blockify() into R_Page0..n
    EX->>TC: insertTable(R)
```

*Figure 2. Sequence diagram for a selection. The same shape applies to every assignment operator: parse, validate against the catalogue, scan inputs through cursors, append to a result CSV, blockify, register.*

If either parsing phase fails the command stops with `SYNTAX ERROR` or `SEMANTIC ERROR: ...` and nothing is executed. A selection that matches no rows prints `Empty Table` and the result relation is unloaded instead of registered.

## 3. Main types

```mermaid
classDiagram
    class ParsedQuery {
        +QueryType queryType
        +string selectionRelationName
        +BinaryOperator selectionBinaryOperator
        +string joinFirstRelationName
        +string joinSecondRelationName
        +string bufferSize
        +string groupByAgg
        +vector projectionColumnList
        +clear()
    }
    class TableCatalogue {
        -unordered_map tables
        +insertTable(Table)
        +deleteTable(name)
        +getTable(name) Table
        +isTable(name) bool
        +isColumnFromTable(col, name) bool
        +print()
    }
    class Table {
        +string tableName
        +string sourceFileName
        +vector columns
        +uint columnCount
        +long rowCount
        +uint blockCount
        +uint maxRowsPerBlock
        +vector rowsPerBlockCount
        +load() bool
        +blockify() bool
        +getCursor() Cursor
        +writeRow(row)
        +renameColumn(from, to)
        +print()
        +makePermanent()
        +unload()
    }
    class Cursor {
        +Page page
        +int pageIndex
        +int pagePointer
        +getNext() vector
        +nextPage(index)
    }
    class BufferManager {
        -deque pages
        +getPage(table, index) Page
        +writePage(table, index, rows, count)
        +deleteFile(name)
    }
    class Page {
        +string pageName
        -vector rows
        +getRow(index) vector
        +writePage()
    }
    class MatrixCatalogue {
        -unordered_map tables
        +insertMatrix(Matrix)
        +getMatrix(name) Matrix
        +isMatrix(name) bool
    }
    class Matrix {
        +string tableName
        +string tempSourceFileName
        +int matrix_size
        +int matrix_block_size
        +load() bool
        +print()
        +makePermanent()
        +unload()
    }

    TableCatalogue "1" o-- "*" Table
    MatrixCatalogue "1" o-- "*" Matrix
    Table ..> Cursor : creates
    Cursor --> Page : current page
    Cursor ..> BufferManager : getPage
    BufferManager "1" o-- "0..BLOCK_COUNT" Page
```

*Figure 3. Class diagram. `ParsedQuery` is a flat record with one group of fields per command. A `Table` owns its schema and statistics but not its rows; rows live in page files and are reached through `Cursor` and `BufferManager`. `Matrix` is a parallel type with its own catalogue and its own tiled file format.*

`Page` objects are value types: the buffer manager returns copies, and a `Cursor` holds its own copy of the current page. The pool is a `deque<Page>` searched linearly by page name; on a miss the page is read from disk, pushed to the back, and the front page is dropped once the pool holds `BLOCK_COUNT` pages.

## 4. Storage layout

| Path | Written by | Format |
| --- | --- | --- |
| `data/<T>.csv` | User, or `EXPORT T` | Header line of column names, then comma-separated integer rows |
| `data/temp/<T>.csv` | Every assignment operator | Result rows before paging (header + `, `-separated rows); deleted on `CLEAR`/`QUIT` |
| `data/temp/<T>_Page<k>` | `Table::blockify` via `BufferManager::writePage` | Up to `maxRowsPerBlock` lines, each a row of space-separated integers |
| `data/temp/<T>_buc_<i>.csv`, `<T>_buc_<i>_Page<k>` | Partition-hash join | Temporary bucket relations, dropped when the join finishes |
| `data/<M>.csv` | User, or `EXPORT MATRIX M` | Headerless N x N grid, comma-separated |
| `data/temp/<M>_<i>_<j>` | `Matrix::load` | One 20x20 tile: 20 lines of 20 space-separated values, zero-padded |
| `data/<name>.ra` | User | Script file checked by `SOURCE` (execution is a stub) |
| `src/log` | `Logger` | Function-level trace of the last session |

There is no catalogue file: schema and statistics are reconstructed from the CSV header on every `LOAD` and kept only in memory.

### Relation lifecycle

```mermaid
stateDiagram-v2
    [*] --> OnDisk: data/T.csv exists
    OnDisk --> Loaded: LOAD T
    [*] --> Temporary: R <- operator
    Loaded --> Loaded: RENAME / used as input
    Temporary --> Temporary: RENAME / used as input
    Temporary --> Exported: EXPORT R
    Loaded --> Exported: EXPORT T
    Exported --> Exported: used as input
    Loaded --> Dropped: CLEAR or QUIT
    Temporary --> Dropped: CLEAR or QUIT
    Exported --> Dropped: CLEAR or QUIT
    Dropped --> [*]
```

*Figure 4. State diagram of a relation. Loaded and derived relations both exist as page files in `data/temp/` for the session. `EXPORT` writes a CSV into `data/`, which is the only state that survives `QUIT`; `CLEAR` and `QUIT` delete the page files.*

`RENAME` only changes the in-memory column list; page files hold no header, so nothing on disk is rewritten until an `EXPORT`.

## 5. Operator algorithms

### Selection, projection, cross product

All three are single-pass scans that write one result row per qualifying input row (selection), per input row (projection) or per input pair (cross product, a tuple-at-a-time nested loop that reopens the inner cursor for each outer row). Projection keeps duplicates. For a cross product, columns present in both inputs are prefixed with the table name (`EMPLOYEE_Ssn`); crossing a table with itself prefixes with `<T>1_` and `<T>2_`.

### Block nested-loop join

```mermaid
flowchart TD
    A["Open cursor on outer relation R"] --> B{"More rows in R?"}
    B -->|"no"| Z["blockify result, register, print cost"]
    B -->|"yes"| C["Read up to (B - 2) x maxRowsPerBlock rows of R into memory"]
    C --> D["Open cursor on inner relation S"]
    D --> E{"More rows in S?"}
    E -->|"no"| B
    E -->|"yes"| F["Read one page worth of S rows"]
    F --> G["Compare every in-memory R row with every S row using the join operator"]
    G --> H["Append matches to result CSV"]
    H --> E
```

*Figure 5. Block nested-loop join. `B - 2` buffers hold a chunk of the outer relation, one buffer streams the inner relation, one is reserved for output. Any comparison operator is supported because every pair in the chunk is tested.*

Reported cost: `b(R) + b(S) * ceil(b(R) / (B - 2)) + b(result)`.

### Partition-hash join

```mermaid
flowchart LR
    R["Relation R"] -->|"key % (B-1)"| RB["R_buc_0 .. R_buc_(B-2)"]
    S["Relation S"] -->|"key % (B-1)"| SB["S_buc_0 .. S_buc_(B-2)"]
    RB --> J["Join bucket i of R with bucket i of S (nested loop)"]
    SB --> J
    J --> O["Result relation"]
    J --> D["Drop all bucket tables"]
```

*Figure 6. Partition-hash join. Both inputs are partitioned into `B - 1` paged bucket relations by the join key modulo `B - 1`; only matching bucket pairs are compared.*

Reported cost: `b(R) + b(S) + b(result) + Σ_i (b(R_i) + b(S_i) + b(R_i) * b(S_i))`. Because partitioning is by modulo, rows that satisfy a non-equality predicate can land in different buckets, so the result is only complete for `==`.

### GROUP BY

One cursor scan maintains `unordered_map<int, int>` from group key to running `MAX`/`MIN`/`SUM` (and a second count map for `AVG`). The map is then emitted as a two-column relation `<g>, <AGG><x>`. Output order follows the hash map. `AVG` values are written as floats and truncated to integers by `stoi` when the result is paged.

### Matrix tiling and in-place cross transpose

`LOAD MATRIX M` reads `data/M.csv` row by row and appends each 20-value slice to tile `M_<rowBlock>_<colBlock>`, padding the last tile row and column with zeros. `CROSS_TRANSPOSE A B` (matrices of equal size) iterates over every tile coordinate `(i, j)`, loads `A_i_j` and `B_j_i`, swaps `A[r][c]` with `B[c][r]` for every cell, and writes both tiles back. After the pass, `A` holds `Bᵀ` and `B` holds `Aᵀ`, with at most two tiles in memory. `EXPORT MATRIX M` walks the tile grid row by row, seeking to the right line in each tile, and rewrites `data/M.csv` without the padding. See [matrix.md](matrix.md) for the original design notes and figures; the sparse-matrix sections there are a design proposal only.

## 6. Unimplemented operators

`executeJOIN`, `executeDISTINCT`, `executeSORT`, `executeINDEX` and `executeSOURCE` are empty functions. Their parsers and semantic checks are complete, so the grammar accepts them, but they create no relation and print nothing. The existing cursor, `writeRow` and `blockify` primitives are sufficient to add an external merge sort (and from it `SORT`, `DISTINCT` and a sort-merge `JOIN`) without changing the storage layer.
