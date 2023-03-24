#include "global.h"
/**
 * @brief
 * SYNTAX: R <- JOIN relation_name1, relation_name2 ON column_name1 bin_op column_name2
 */
bool syntacticParseNESTEDJOIN()
{
    logger.log("syntacticParseNESTEDJOIN");
    if (tokenizedQuery.size() != 13 || tokenizedQuery[7] != "ON" || tokenizedQuery[3] != "USING" || tokenizedQuery[11] != "BUFFER")
    {
        cout << "SYNTAC ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = NJOIN;
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

bool semanticParseNESTEDJOIN()
{
    logger.log("semanticParseNESTEDJOIN");

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

bool operatorJoin(int val1, int val2, BinaryOperator opertr)
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

void executeNESTEDJOIN()
{
    logger.log("executeNESTEDJOIN");

    Table table1 = *(tableCatalogue.getTable(parsedQuery.joinFirstRelationName));
    Table table2 = *(tableCatalogue.getTable(parsedQuery.joinSecondRelationName));

    int col_index1 = table1.getColumnIndex(parsedQuery.joinFirstColumnName);
    int col_index2 = table2.getColumnIndex(parsedQuery.joinSecondColumnName);

    int buffer_size = stoi(parsedQuery.bufferSize);

    vector<string> result_cols;
    result_cols.insert(result_cols.end(), table1.columns.begin(), table1.columns.end());
    result_cols.insert(result_cols.end(), table2.columns.begin(), table2.columns.end());

    Table *resultantTable = new Table(parsedQuery.joinResultRelationName, result_cols);
    vector<int> resultantRow;
    resultantRow.reserve(resultantTable->columnCount);

    Cursor cursor1 = table1.getCursor();

    vector<int> row1 = cursor1.getNext();

    int total_buffer_row_size = (buffer_size - 2) * table1.maxRowsPerBlock;
    while (!row1.empty())
    {
        vector<vector<int>> first_relation_rows;
        while (!row1.empty() && first_relation_rows.size() != total_buffer_row_size)
        {
            first_relation_rows.push_back(row1);
            row1 = cursor1.getNext();
        }
        Cursor cursor2 = table2.getCursor();
        vector<int> row2;
        row2 = cursor2.getNext();
        while (!row2.empty())
        {
            vector<vector<int>> second_relation_rows;
            while (!row2.empty() && second_relation_rows.size() != table2.maxRowsPerBlock)
            {
                second_relation_rows.push_back(row2);
                row2 = cursor2.getNext();
            }

            for (auto first_rel : first_relation_rows)
            {
                for (auto second_rel : second_relation_rows)
                {
                    if (operatorJoin(first_rel[col_index1], second_rel[col_index2], parsedQuery.joinBinaryOperator))
                    {
                        resultantRow = first_rel;
                        resultantRow.insert(resultantRow.end(), second_rel.begin(), second_rel.end());
                        resultantTable->writeRow<int>(resultantRow);
                    }
                }
            }
        }
    }
    resultantTable->blockify();
    tableCatalogue.insertTable(resultantTable);
    int total_block_access = table1.blockCount + resultantTable->blockCount;
    total_block_access += (table2.blockCount * ceil((table1.blockCount * 1.0) / (buffer_size - 2)));
    cout << "Total number of block accesses are " << total_block_access << endl;

    return;
}