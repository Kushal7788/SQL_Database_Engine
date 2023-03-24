#include "matrix.h"

/**
 * @brief The TableCatalogue acts like an index of tables existing in the
 * system. Everytime a table is added(removed) to(from) the system, it needs to
 * be added(removed) to(from) the tableCatalogue.
 *
 */
class MatrixCatalogue
{

    unordered_map<string, Matrix *> tables;

public:
    MatrixCatalogue() {}
    void insertMatrix(Matrix *matrix);
    void deleteMatrix(string tableName);
    Matrix *getMatrix(string tableName);
    bool isMatrix(string tableName);
    void print();
    ~MatrixCatalogue();
};