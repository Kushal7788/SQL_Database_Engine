#include "tableCatalogue.h"
#include "matrixCatalogue.h"

using namespace std;

enum QueryType
{
    CLEAR,
    CROSS,
    CROSSTRANS,
    DISTINCT,
    EXPORT,
    EXPORTM,
    INDEX,
    JOIN,
    NJOIN,
    PHJOIN,
    GROUPBY,
    LIST,
    LOAD,
    LOADM,
    PRINT,
    PRINTM,
    PROJECTION,
    RENAME,
    SELECTION,
    SORT,
    SOURCE,
    UNDETERMINED
};

enum BinaryOperator
{
    LESS_THAN,
    GREATER_THAN,
    LEQ,
    GEQ,
    EQUAL,
    NOT_EQUAL,
    NO_BINOP_CLAUSE
};

enum SortingStrategy
{
    ASC,
    DESC,
    NO_SORT_CLAUSE
};

enum SelectType
{
    COLUMN,
    INT_LITERAL,
    NO_SELECT_CLAUSE
};

class ParsedQuery
{

public:
    QueryType queryType = UNDETERMINED;

    string clearRelationName = "";

    string crossResultRelationName = "";
    string crossFirstRelationName = "";
    string crossSecondRelationName = "";

    string distinctResultRelationName = "";
    string distinctRelationName = "";

    string exportRelationName = "";

    IndexingStrategy indexingStrategy = NOTHING;
    string indexColumnName = "";
    string indexRelationName = "";

    BinaryOperator joinBinaryOperator = NO_BINOP_CLAUSE;
    string joinResultRelationName = "";
    string joinFirstRelationName = "";
    string joinSecondRelationName = "";
    string joinFirstColumnName = "";
    string joinSecondColumnName = "";
    string bufferSize = "";

    string groupByRelation = "";
    string groupByRelationCol = "";
    string groupByResultRelation = "";
    string groupByAgg = "";
    string groupByAggCol = "";

    string loadRelationName = "";

    string printRelationName = "";

    string projectionResultRelationName = "";
    vector<string> projectionColumnList;
    string projectionRelationName = "";

    string renameFromColumnName = "";
    string renameToColumnName = "";
    string renameRelationName = "";

    SelectType selectType = NO_SELECT_CLAUSE;
    BinaryOperator selectionBinaryOperator = NO_BINOP_CLAUSE;
    string selectionResultRelationName = "";
    string selectionRelationName = "";
    string selectionFirstColumnName = "";
    string selectionSecondColumnName = "";
    int selectionIntLiteral = 0;

    SortingStrategy sortingStrategy = NO_SORT_CLAUSE;
    string sortResultRelationName = "";
    string sortColumnName = "";
    string sortRelationName = "";

    string sourceFileName = "";

    ParsedQuery();
    void clear();
};

bool syntacticParse();
bool syntacticParseCLEAR();
bool syntacticParseCROSS();
bool syntacticParseCROSSTRANSPOSE();
bool syntacticParseDISTINCT();
bool syntacticParseEXPORT();
bool syntacticParseEXPORTMATRIX();
bool syntacticParseINDEX();
bool syntacticParseJOIN();
bool syntacticParseNESTEDJOIN();
bool syntacticParsePARTHASHJOIN();
bool syntacticParseLIST();
bool syntacticParseLOAD();
bool syntacticParseLOADMATRIX();
bool syntacticParsePRINT();
bool syntacticParsePRINTMATRIX();
bool syntacticParsePROJECTION();
bool syntacticParseRENAME();
bool syntacticParseSELECTION();
bool syntacticParseSORT();
bool syntacticParseSOURCE();
bool syntacticParseGROUPBY();

bool isFileExists(string tableName);
bool isQueryFile(string fileName);
