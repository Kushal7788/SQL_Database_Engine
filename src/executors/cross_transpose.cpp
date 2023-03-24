#include "global.h"

/**
 * @brief
 * SYNTAX: R <- CROSS relation_name relation_name
 */
bool syntacticParseCROSSTRANSPOSE()
{
    logger.log("syntacticParseCROSSTRANSPOSE");
    if (tokenizedQuery.size() != 3)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = CROSSTRANS;
    parsedQuery.crossFirstRelationName = tokenizedQuery[1];
    parsedQuery.crossSecondRelationName = tokenizedQuery[2];
    return true;
}

bool semanticParseCROSSTRANSPOSE()
{
    logger.log("semanticParseCROSSTRANSPOSE");
    // Both matrix must exist
    if (!matrixCatalogue.isMatrix(parsedQuery.crossFirstRelationName) || !matrixCatalogue.isMatrix(parsedQuery.crossSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Cross Matrices don't exist" << endl;
        return false;
    }
    Matrix *m1, *m2;
    m1 = matrixCatalogue.getMatrix(parsedQuery.crossFirstRelationName);
    m2 = matrixCatalogue.getMatrix(parsedQuery.crossSecondRelationName);
    if (m1->matrix_size != m2->matrix_size)
    {
        cout << "SEMANTIC ERROR: Cross Matrices have different sizes" << endl;
        return false;
    }
    return true;
}

vector<vector<string>> getMatrixFromTemp(string file_path, Matrix *matrix)
{
    fstream mat(file_path, ios::in);
    string line;
    vector<vector<string>> ans;
    while (getline(mat, line))
    {
        vector<string> line_vec = matrix->split(line, ' ');
        ans.push_back(line_vec);
    }
    mat.close();
    return ans;
}

void write_pages(vector<vector<string>> &m, string file_path)
{
    fstream mat(file_path, ios::out);
    for (int i = 0; i < m.size(); i++)
    {
        for (int j = 0; j < m.size(); j++)
        {
            mat << m[i][j] << " ";
        }
        mat << endl;
    }
    mat.close();
}

void swap_pages(vector<vector<string>> &m1, vector<vector<string>> &m2)
{
    for (int i = 0; i < m1.size(); i++)
    {
        for (int j = 0; j < m2.size(); j++)
        {
            string temp_val = m1[i][j];
            m1[i][j] = m2[j][i];
            m2[j][i] = temp_val;
        }
    }
}

void executeCROSSTRANSPOSE()
{
    logger.log("executeCROSSTRANSPOSE");

    Matrix *matrix1 = matrixCatalogue.getMatrix(parsedQuery.crossFirstRelationName);
    Matrix *matrix2 = matrixCatalogue.getMatrix(parsedQuery.crossSecondRelationName);
    int total_row_blocks = ceil(float((matrix1->matrix_size * 1.0) / matrix1->matrix_block_size));
    for (int i = 0; i < total_row_blocks; i++)
    {
        for (int j = 0; j < total_row_blocks; j++)
        {
            string m1_file_name = matrix1->tempSourceFileName + "_" + to_string(i) + "_" + to_string(j);
            string m2_file_name = matrix2->tempSourceFileName + "_" + to_string(j) + "_" + to_string(i);
            vector<vector<string>> m1 = getMatrixFromTemp(m1_file_name, matrix1);
            vector<vector<string>> m2 = getMatrixFromTemp(m2_file_name, matrix2);
            swap_pages(m1, m2);
            write_pages(m1, m1_file_name);
            write_pages(m2, m2_file_name);
        }
    }
    return;
}