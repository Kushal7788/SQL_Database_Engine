#include "global.h"
/**
 * @brief
 * SYNTAX: LOAD relation_name
 */

bool syntacticParseGROUPBY()
{
    logger.log("syntacticParseGROUPBY");
    if (tokenizedQuery.size() != 9 || tokenizedQuery[2] != "GROUP" || tokenizedQuery[3] != "BY" || tokenizedQuery[5] != "FROM" || tokenizedQuery[7] != "RETURN")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    string agg_fun = tokenizedQuery[8].substr(0, 3);
    if (agg_fun != "MAX" && agg_fun != "MIN" && agg_fun != "SUM" && agg_fun != "AVG")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    string agg_col = tokenizedQuery[8].substr(4, tokenizedQuery[8].size() - 5);
    parsedQuery.queryType = GROUPBY;
    parsedQuery.groupByRelation = tokenizedQuery[6];
    parsedQuery.groupByRelationCol = tokenizedQuery[4];
    parsedQuery.groupByResultRelation = tokenizedQuery[0];
    parsedQuery.groupByAgg = agg_fun;
    parsedQuery.groupByAggCol = agg_col;
    return true;
}

bool semanticParseGROUPBY()
{
    logger.log("semanticParseGROUPBY");
    if (tableCatalogue.isTable(parsedQuery.groupByResultRelation))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }
    if (!tableCatalogue.isTable(parsedQuery.groupByRelation))
    {
        cout << "SEMANTIC ERROR: Relation don't exist" << endl;
        return false;
    }
    if (!tableCatalogue.isColumnFromTable(parsedQuery.groupByRelationCol, parsedQuery.groupByRelation) || !tableCatalogue.isColumnFromTable(parsedQuery.groupByAggCol, parsedQuery.groupByRelation))
    {
        cout << "SEMANTIC ERROR: Column don't exist in the given relation" << endl;
        return false;
    }
    return true;
}

void executeGROUPBY()
{
    logger.log("executeGROUPBY");

    Table table = *(tableCatalogue.getTable(parsedQuery.groupByRelation));
    Cursor cursor = table.getCursor();

    unordered_map<int, int> relation_map;

    vector<string> columns;
    columns.push_back(parsedQuery.groupByRelationCol);
    columns.push_back(parsedQuery.groupByAgg + parsedQuery.groupByAggCol);

    Table *resultantTable = new Table(parsedQuery.groupByResultRelation, columns);

    int group_col_ind, group_attr_col_ind;
    for (int i = 0; i < table.columnCount; i++)
    {
        if (table.columns[i] == parsedQuery.groupByRelationCol)
        {
            group_col_ind = i;
        }
        if (table.columns[i] == parsedQuery.groupByAggCol)
        {
            group_attr_col_ind = i;
        }
    }
    vector<int> relation_row = cursor.getNext();

    if (parsedQuery.groupByAgg == "MAX")
    {
        vector<int> resultantRow;
        resultantRow.reserve(resultantTable->columnCount);
        while (!relation_row.empty())
        {
            if (relation_map.find(relation_row[group_col_ind]) == relation_map.end())
            {
                relation_map[relation_row[group_col_ind]] = relation_row[group_attr_col_ind];
            }
            else
            {
                if (relation_map[relation_row[group_col_ind]] < relation_row[group_attr_col_ind])
                {
                    relation_map[relation_row[group_col_ind]] = relation_row[group_attr_col_ind];
                }
            }
            relation_row = cursor.getNext();
        }
        for (auto mp : relation_map)
        {
            vector<int> result_row_add;
            result_row_add.push_back(mp.first);
            result_row_add.push_back(mp.second);
            resultantRow = result_row_add;
            resultantTable->writeRow<int>(resultantRow);
        }
    }
    else if (parsedQuery.groupByAgg == "MIN")
    {
        vector<int> resultantRow;
        resultantRow.reserve(resultantTable->columnCount);
        while (!relation_row.empty())
        {
            if (relation_map.find(relation_row[group_col_ind]) == relation_map.end())
            {
                relation_map[relation_row[group_col_ind]] = relation_row[group_attr_col_ind];
            }
            else
            {
                if (relation_map[relation_row[group_col_ind]] > relation_row[group_attr_col_ind])
                {
                    relation_map[relation_row[group_col_ind]] = relation_row[group_attr_col_ind];
                }
            }
            relation_row = cursor.getNext();
        }
        for (auto mp : relation_map)
        {
            vector<int> result_row_add;
            result_row_add.push_back(mp.first);
            result_row_add.push_back(mp.second);
            resultantRow = result_row_add;
            resultantTable->writeRow<int>(resultantRow);
        }
    }
    else if (parsedQuery.groupByAgg == "SUM")
    {
        vector<int> resultantRow;
        resultantRow.reserve(resultantTable->columnCount);
        while (!relation_row.empty())
        {
            if (relation_map.find(relation_row[group_col_ind]) == relation_map.end())
            {
                relation_map[relation_row[group_col_ind]] = relation_row[group_attr_col_ind];
            }
            else
            {
                relation_map[relation_row[group_col_ind]] += relation_row[group_attr_col_ind];
            }
            relation_row = cursor.getNext();
        }
        for (auto mp : relation_map)
        {
            vector<int> result_row_add;
            result_row_add.push_back(mp.first);
            result_row_add.push_back(mp.second);
            resultantRow = result_row_add;
            resultantTable->writeRow<int>(resultantRow);
        }
    }
    else if (parsedQuery.groupByAgg == "AVG")
    {
        vector<float> resultantRow;
        unordered_map<int, int> counter;
        resultantRow.reserve(resultantTable->columnCount);
        while (!relation_row.empty())
        {
            if (relation_map.find(relation_row[group_col_ind]) == relation_map.end())
            {
                relation_map[relation_row[group_col_ind]] = relation_row[group_attr_col_ind];
                counter[relation_row[group_col_ind]] = 1;
            }
            else
            {
                relation_map[relation_row[group_col_ind]] += relation_row[group_attr_col_ind];
                counter[relation_row[group_col_ind]] += 1;
            }
            relation_row = cursor.getNext();
        }
        for (auto mp : relation_map)
        {
            vector<float> result_row_add;
            result_row_add.push_back(mp.first);
            float val = (mp.second * 1.0) / counter[mp.first];
            result_row_add.push_back(val);
            logger.log(to_string(mp.first) + " " + to_string(val) + " " + to_string(result_row_add[1]));
            resultantRow = result_row_add;
            resultantTable->writeRow<float>(resultantRow);
        }
    }
    resultantTable->blockify();
    tableCatalogue.insertTable(resultantTable);
    cout << "Total number of block accesses are " << resultantTable->blockCount + table.blockCount << endl;
    return;
}