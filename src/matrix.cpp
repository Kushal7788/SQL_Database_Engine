#include "global.h"

/**
 * @brief Construct a new Table:: Table object
 *
 */
Matrix::Matrix()
{
    logger.log("Matrix::Matrix");
}

/**
 * @brief Construct a new Table:: Table object used in the case where the data
 * file is available and LOAD command has been called. This command should be
 * followed by calling the load function;
 *
 * @param tableName
 */
Matrix::Matrix(string tableName)
{
    logger.log("Matrix::Matrix");
    this->sourceFileName = "../data/" + tableName + ".csv";
    this->tempSourceFileName = "../data/temp/" + tableName;
    this->tableName = tableName;
}

/**
 * @brief Construct a new Table:: Table object used when an assignment command
 * is encountered. To create the table object both the table name and the
 * columns the table holds should be specified.
 *
 * @param tableName
 * @param columns
 */
Matrix::Matrix(string tableName, vector<string> columns)
{
    logger.log("Matrix::Matrix");
    this->sourceFileName = "../data/temp/" + tableName + ".csv";
    this->tableName = tableName;
    this->columns = columns;
    this->columnCount = columns.size();
    this->maxRowsPerBlock = (uint)((BLOCK_SIZE * 1000) / (sizeof(int) * columnCount));
    this->writeRow<string>(columns);
}

/**
 * @brief The load function is used when the LOAD command is encountered. It
 * reads data from the source file, splits it into blocks and updates table
 * statistics.
 *
 * @return true if the table has been successfully loaded
 * @return false if an error occurred
 */

vector<string> Matrix::split(string line, char delimiter)
{
    stringstream ss(line);
    vector<string> tokens;
    string token;
    while (getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

bool Matrix::load()
{
    logger.log("Matrix::load");

    // this->matrix_block_size = 0;
    int total_row_blocks = 0;
    fstream matrix_in(this->sourceFileName, ios::in);
    string line;
    int row = 0;
    int block_row = 0;
    while (getline(matrix_in, line))
    {
        vector<string> tokens = split(line, ',');
        this->matrix_size = tokens.size();
        total_row_blocks = ceil(float((this->matrix_size * 1.0) / this->matrix_block_size));

        for (int i = 0; i < total_row_blocks; i++)
        {
            fstream temp_block(this->tempSourceFileName + "_" + to_string(block_row) + "_" + to_string(i), ios::app);
            for (int j = 0; j < this->matrix_block_size; j++)
            {
                if (((this->matrix_block_size) * i) + j >= tokens.size())
                {
                    temp_block << "0 ";
                }
                else
                {
                    temp_block << tokens[((this->matrix_block_size) * i) + j] + " ";
                }
            }
            temp_block << endl;
            temp_block.close();
        }
        row++;
        if (row % this->matrix_block_size == 0)
        {
            block_row++;
        }
    }
    if (this->matrix_size % this->matrix_block_size != 0)
    {
        int zero_pad = this->matrix_block_size - (this->matrix_size % this->matrix_block_size);
        for (int i = 0; i < total_row_blocks; i++)
        {
            fstream temp_block(this->tempSourceFileName + "_" + to_string(total_row_blocks - 1) + "_" + to_string(i), ios::app);
            for (int j = 0; j < zero_pad; j++)
            {
                for (int k = 0; k < this->matrix_block_size; k++)
                {
                    temp_block << "0 ";
                }
                temp_block << endl;
            }
            temp_block.close();
        }
    }
    matrix_in.close();
    return true;
}

/**
 * @brief Function extracts column names from the header line of the .csv data
 * file.
 *
 * @param line
 * @return true if column names successfully extracted (i.e. no column name
 * repeats)
 * @return false otherwise
 */
bool Matrix::extractColumnNames(string firstLine)
{
    logger.log("Matrix::extractColumnNames");
    unordered_set<string> columnNames;
    string word;
    stringstream s(firstLine);
    while (getline(s, word, ','))
    {
        word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
        if (columnNames.count(word))
            return false;
        columnNames.insert(word);
        this->columns.emplace_back(word);
    }
    this->columnCount = this->columns.size();
    this->maxRowsPerBlock = (uint)((BLOCK_SIZE * 1000) / (sizeof(int) * this->columnCount));
    return true;
}

/**
 * @brief Function prints the first few rows of the table. If the table contains
 * more rows than PRINT_COUNT, exactly PRINT_COUNT rows are printed, else all
 * the rows are printed.
 *
 */
void Matrix::print()
{
    logger.log("Matrix::print");
    fstream temp_mat(this->tempSourceFileName + "_0_0", ios::in);
    string line;
    cout << endl;
    if (this->matrix_size <= this->matrix_block_size)
    {
        for (int i = 0; i < this->matrix_size; i++)
        {
            getline(temp_mat, line);
            vector<string> tokens = split(line, ' ');
            for (int j = 0; j < this->matrix_size; j++)
            {
                cout << tokens[j] << " ";
            }
            cout << endl;
        }
    }
    else
    {
        for (int i = 0; i < this->matrix_block_size; i++)
        {
            getline(temp_mat, line);
            vector<string> tokens = split(line, ' ');
            for (int j = 0; j < this->matrix_block_size; j++)
            {
                cout << tokens[j] << " ";
            }
            cout << endl;
        }
    }
    temp_mat.close();
}

/**
 * @brief called when EXPORT command is invoked to move source file to "data"
 * folder.
 *
 */
void Matrix::makePermanent()
{
    logger.log("Matrix::makePermanent");

    if (!isPermanent())
    {
        cout << "Matrix file no longer exists\n";
        return;
    }

    int total_row_blocks = ceil(float((this->matrix_size * 1.0) / this->matrix_block_size));
    fstream mat_write(this->sourceFileName, ios::out);
    int row_count = 0;
    int row_block_count = 0;
    unordered_map<int, int> mp;
    while (row_block_count < total_row_blocks)
    {
        int col_count = 0;
        for (int j = 0; j < total_row_blocks; j++)
        {
            string line;
            fstream mat(this->tempSourceFileName + "_" + to_string(row_block_count) + "_" + to_string(j), ios::in);
            mat.seekg(mp[j], ios::beg);
            getline(mat, line);
            vector<string> tokens = split(line, ' ');

            for (int k = 0; k < tokens.size(); k++)
            {
                if (col_count >= this->matrix_size)
                {
                    break;
                }
                if (col_count != this->matrix_size - 1)
                    mat_write << tokens[k] << ",";
                else
                    mat_write << tokens[k];
                col_count++;
            }
            mp[j] += line.size() + 1;
            mat.close();
        }
        row_count++;
        mat_write << endl;
        if ((row_count % this->matrix_block_size) == 0)
        {
            row_block_count++;
            mp.clear();
        }
        if (row_count >= this->matrix_size)
        {
            break;
        }
    }
    mat_write.close();
}

/**
 * @brief Function to check if table is already exported
 *
 * @return true if exported
 * @return false otherwise
 */
bool Matrix::isPermanent()
{
    logger.log("Matrix::isPermanent");
    if (this->sourceFileName == "../data/" + this->tableName + ".csv")
        return true;
    return false;
}

/**
 * @brief The unload function removes the table from the database by deleting
 * all temporary files created as part of this table
 *
 */
void Matrix::unload()
{
    logger.log("Matrix::~unload");
    int total_row_blocks = ceil(float((this->matrix_size * 1.0) / this->matrix_block_size));

    for (int i = 0; i < total_row_blocks; i++)
    {
        for (int j = 0; j < total_row_blocks; j++)
        {
            bufferManager.deleteFile(this->tempSourceFileName + "_" + to_string(i) + "_" + to_string(j));
        }
    }
    // for (int pageCounter = 0; pageCounter < this->blockCount; pageCounter++)
    //     bufferManager.deleteFile(this->tableName, pageCounter);
    // if (!isPermanent())
    //     bufferManager.deleteFile(this->sourceFileName);
}

/**
 * @brief Function that returns a cursor that reads rows from this table
 *
 * @return Cursor
 */
Cursor Matrix::getCursor()
{
    logger.log("Matrix::getCursor");
    Cursor cursor(this->tableName, 0);
    return cursor;
}
/**
 * @brief Function that returns the index of column indicated by columnName
 *
 * @param columnName
 * @return int
 */
int Matrix::getColumnIndex(string columnName)
{
    logger.log("Matrix::getColumnIndex");
    for (int columnCounter = 0; columnCounter < this->columnCount; columnCounter++)
    {
        if (this->columns[columnCounter] == columnName)
            return columnCounter;
    }
}
