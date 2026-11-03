# Command Grammar

This is the complete grammar accepted by the parser in `src/syntacticParser.cpp`
and the per-command `syntacticParse*` functions in `src/executors/` and
`src/clear.cpp`.

Tokenisation rule: a command line is split on whitespace **and commas**
(regex `[^\s,]+`), so `PROJECT a, b` and `PROJECT a b` are equivalent, and
`MAX(Salary)` stays a single token. Keywords are case-sensitive and must be
upper case. Identifiers cannot contain spaces or commas.

## Grammar

```
statement -> relation_name <- assignment_statement
           | non_assignment_statement
           | matrix_statement
           | QUIT

assignment_statement -> cross_product_statement
                      | selection_statement
                      | projection_statement
                      | nested_join_statement
                      | partition_hash_join_statement
                      | group_by_statement
                      | join_statement          (parsed and validated only)
                      | distinct_statement      (parsed and validated only)
                      | sort_statement          (parsed and validated only)

non_assignment_statement -> load_statement
                          | list_statement
                          | print_statement
                          | rename_statement
                          | export_statement
                          | clear_statement
                          | index_statement     (parsed and validated only)
                          | source_statement    (parsed and validated only)

matrix_statement -> LOAD MATRIX matrix_name
                  | PRINT MATRIX matrix_name
                  | EXPORT MATRIX matrix_name
                  | CROSS_TRANSPOSE matrix_name matrix_name

cross_product_statement -> CROSS relation_name relation_name

selection_statement -> SELECT condition FROM relation_name
condition -> column_name bin_op column_name
           | column_name bin_op int_literal

projection_statement -> PROJECT column_list FROM relation_name
column_list -> column_list, column_name | column_name

nested_join_statement ->
    JOIN USING NESTED relation_name, relation_name
    ON column_name bin_op column_name BUFFER int_literal

partition_hash_join_statement ->
    JOIN USING PARTHASH relation_name, relation_name
    ON column_name bin_op column_name BUFFER int_literal

group_by_statement ->
    GROUP BY column_name FROM relation_name RETURN aggregate(column_name)
aggregate -> MAX | MIN | SUM | AVG

join_statement -> JOIN relation_name, relation_name ON column_name bin_op column_name
distinct_statement -> DISTINCT relation_name
sort_statement -> SORT relation_name BY column_name IN sorting_order
sorting_order -> ASC | DESC

load_statement   -> LOAD relation_name
list_statement   -> LIST TABLES
print_statement  -> PRINT relation_name
rename_statement -> RENAME column_name TO column_name FROM relation_name
export_statement -> EXPORT relation_name
clear_statement  -> CLEAR relation_name
index_statement  -> INDEX ON column_name FROM relation_name USING indexing_strategy
indexing_strategy -> HASH | BTREE | NOTHING
source_statement -> SOURCE file_name

bin_op -> > | < | == | != | <= | >= | => | =<
```

## Semantic rules

Checked in the `semanticParse*` functions before anything executes:

- The result relation of an assignment must not already exist.
- Every input relation must already be loaded (or created by an earlier
  assignment).
- Every referenced column must exist in its relation. For joins, the first
  column belongs to the first relation and the second column to the second.
- `BUFFER` for `NESTED` / `PARTHASH` joins must be at least 3.
- `RENAME` target column must not already exist.
- `LOAD` / `LOAD MATRIX` require `data/<name>.csv` to exist.
- `SOURCE` requires `data/<name>.ra` to exist.
- `CROSS_TRANSPOSE` requires both matrices to be loaded and of equal size.

## Statements that parse but do not execute

`JOIN ... ON ...` (without `USING`), `DISTINCT`, `SORT`, `INDEX` and `SOURCE`
pass syntactic and semantic checks, but their `execute*` functions are empty
stubs inherited from the SimpleRA skeleton. They produce no output and create
no relation.
