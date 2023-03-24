#include "global.h"
/**
 * @brief
 * SYNTAX: R <- JOIN relation_name1, relation_name2 ON column_name1 bin_op column_name2
 */
bool syntacticParsePARTHASHJOIN()
{
    logger.log("syntacticParsePARTHASHJOIN");
    if (tokenizedQuery.size() != 13 || tokenizedQuery[7] != "ON" || tokenizedQuery[3] != "USING" || tokenizedQuery[11] != "BUFFER")
    {
        cout << "SYNTAC ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = PHJOIN;
    parsedQuery.joinResultRelationName = tokenizedQuery[0];
    parsedQuery.joinFirstRelationName = tokenizedQuery[5];
    parsedQuery.joinSecondRelationName = tokenizedQuery[6];
    parsedQuery.joinFirstColumnName = tokenizedQuery[8];
    parsedQuery.joinSecondColumnName = tokenizedQuery[10];
    parsedQuery.bufferSize = tokenizedQuery[12];


    for (int i = 0; i < parsedQuery.bufferSize.length(); i++)
    {
        if (!isdigit(parsedQuery.bufferSize[i]))
        {
            cout << "SYNTAC ERROR" << endl;
            return false;
        }
    }

    string binaryOperator = tokenizedQuery[9];
    if (binaryOperator == "<")
        parsedQuery.joinBinaryOperator = LESS_THAN;
    else if (binaryOperator == ">")
        parsedQuery.joinBinaryOperator = GREATER_THAN;
    else if (binaryOperator == ">=" || binaryOperator == "=>")
        parsedQuery.joinBinaryOperator = GEQ;
    else if (binaryOperator == "<=" || binaryOperator == "=<")
        parsedQuery.joinBinaryOperator = LEQ;
    else if (binaryOperator == "==")
        parsedQuery.joinBinaryOperator = EQUAL;
    else if (binaryOperator == "!=")
        parsedQuery.joinBinaryOperator = NOT_EQUAL;
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    return true;
}

bool semanticParsePARTHASHJOIN()
{
    logger.log("semanticParsePARTHASHJOIN");

    if (tableCatalogue.isTable(parsedQuery.joinResultRelationName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }

    if (!tableCatalogue.isTable(parsedQuery.joinFirstRelationName) || !tableCatalogue.isTable(parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    if (!tableCatalogue.isColumnFromTable(parsedQuery.joinFirstColumnName, parsedQuery.joinFirstRelationName) || !tableCatalogue.isColumnFromTable(parsedQuery.joinSecondColumnName, parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }
    if (stoi(parsedQuery.bufferSize) < 3)
    {
        cout << "SEMANTIC ERROR: Buffer Size cannot be less than 3" << endl;
        return false;
    }
    return true;
}

bool operatorJoinPH(int val1, int val2, BinaryOperator opertr)
{
    if (opertr == LESS_THAN)
        return val1 < val2;
    else if (opertr == GREATER_THAN)
        return val1 > val2;
    else if (opertr == GEQ)
        return val1 >= val2;
    else if (opertr == LEQ)
        return val1 <= val2;
    else if (opertr == EQUAL)
        return val1 == val2;
    else if (opertr == NOT_EQUAL)
        return val1 != val2;
    else
        return false;
}

vector<int> create_partition(Table table, int mod_val, int col_index)
{
    vector<Table *> buckets;
    for (int i = 0; i < mod_val; i++)
    {
        buckets.push_back(new Table(table.tableName + "_buc_" + to_string(i), table.columns));
    }
    Cursor cursor = table.getCursor();
    vector<int> row = cursor.getNext();
    vector<int> resultantRow;
    resultantRow.reserve(table.columnCount);
    int curr_cntr = 0;
    while (!row.empty())
    {
        resultantRow = row;
        int row_bucket_no = row[col_index] % mod_val;
        buckets[row_bucket_no]->writeRow<int>(resultantRow);
        row = cursor.getNext();
    }
    vector<int> bucket_blocks;
    for (int i = 0; i < mod_val; i++)
    {
        buckets[i]->blockify();
        tableCatalogue.insertTable(buckets[i]);
        bucket_blocks.push_back(buckets[i]->blockCount);
    }
    return bucket_blocks;
}

Table *joinPartition(Table table1, Table table2, int mod_val, int col_index1, int col_index2, BinaryOperator opertr)
{
    vector<string> result_cols;
    result_cols.insert(result_cols.end(), table1.columns.begin(), table1.columns.end());
    result_cols.insert(result_cols.end(), table2.columns.begin(), table2.columns.end());

    Table *resultantTable = new Table(parsedQuery.joinResultRelationName, result_cols);

    vector<int> resultantRow;
    resultantRow.reserve(resultantTable->columnCount);
    int curr_cntr = 0;

    for (int i = 0; i < mod_val; i++)
    {
        Table first_rel_bucket = *(tableCatalogue.getTable(table1.tableName + "_buc_" + to_string(i)));
        Table second_rel_bucket = *(tableCatalogue.getTable(table2.tableName + "_buc_" + to_string(i)));

        if (first_rel_bucket.rowCount && second_rel_bucket.rowCount)
        {
            Cursor cursor1 = first_rel_bucket.getCursor();
            vector<int> row1 = cursor1.getNext();
            while (!row1.empty())
            {
                Cursor cursor2 = second_rel_bucket.getCursor();
                vector<int> row2 = cursor2.getNext();
                while (!row2.empty())
                {
                    if (operatorJoinPH(row1[col_index1], row2[col_index2], opertr))
                    {
                        resultantRow = row1;
                        resultantRow.insert(resultantRow.end(), row2.begin(), row2.end());
                        resultantTable->writeRow<int>(resultantRow);
                    }
                    row2 = cursor2.getNext();
                }
                row1 = cursor1.getNext();
            }
        }
    }
    for (int i = 0; i < mod_val; i++)
    {
        tableCatalogue.deleteTable(table1.tableName + "_buc_" + to_string(i));
        tableCatalogue.deleteTable(table2.tableName + "_buc_" + to_string(i));
    }
    resultantTable->blockify();
    tableCatalogue.insertTable(resultantTable);
    return resultantTable;
}

void executePARTHASHJOIN()
{
    logger.log("executePARTHASHJOIN");

    Table table1 = *(tableCatalogue.getTable(parsedQuery.joinFirstRelationName));
    Table table2 = *(tableCatalogue.getTable(parsedQuery.joinSecondRelationName));

    int col_index1 = table1.getColumnIndex(parsedQuery.joinFirstColumnName);
    int col_index2 = table2.getColumnIndex(parsedQuery.joinSecondColumnName);

    int buffer_size = stoi(parsedQuery.bufferSize);

    vector<int> first_part_blocks, second_part_blocks;
    first_part_blocks = create_partition(table1, buffer_size - 1, col_index1);
    second_part_blocks = create_partition(table2, buffer_size - 1, col_index2);

    Table *resultantTable = joinPartition(table1, table2, buffer_size - 1, col_index1, col_index2, parsedQuery.joinBinaryOperator);

    int total_block_access = table1.blockCount + table2.blockCount + resultantTable->blockCount;
    for (int i = 0; i < buffer_size - 1; i++)
    {
        total_block_access += first_part_blocks[i];
        total_block_access += second_part_blocks[i];
        total_block_access += (first_part_blocks[i] * second_part_blocks[i]);
    }
    cout << "Total number of block accesses are " << total_block_access << endl;

    return;
}