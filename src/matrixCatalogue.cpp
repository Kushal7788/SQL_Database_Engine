#include "global.h"

void MatrixCatalogue::insertMatrix(Matrix *table)
{
    logger.log("MatrixCatalogue::~insertMatrix");
    this->tables[table->tableName] = table;
}
void MatrixCatalogue::deleteMatrix(string tableName)
{
    logger.log("MatrixCatalogue::deleteMatrix");
    this->tables[tableName]->unload();
    delete this->tables[tableName];
    this->tables.erase(tableName);
}
Matrix *MatrixCatalogue::getMatrix(string tableName)
{
    logger.log("MatrixCatalogue::getMatrix");
    Matrix *table = this->tables[tableName];
    return table;
}
bool MatrixCatalogue::isMatrix(string tableName)
{
    logger.log("MatrixCatalogue::isMatrix");
    if (this->tables.count(tableName))
        return true;
    return false;
}

void MatrixCatalogue::print()
{
    logger.log("MatrixCatalogue::print");
    cout << "\nRELATIONS" << endl;

    int rowCount = 0;
    for (auto rel : this->tables)
    {
        cout << rel.first << endl;
        rowCount++;
    }
    printRowCount(rowCount);
}

MatrixCatalogue::~MatrixCatalogue()
{
    logger.log("MatrixCatalogue::~MatrixCatalogue");
    for (auto table : this->tables)
    {
        table.second->unload();
        delete table.second;
    }
}
