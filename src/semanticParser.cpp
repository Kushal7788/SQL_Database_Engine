#include"global.h"

bool semanticParse(){
    logger.log("semanticParse");
    switch(parsedQuery.queryType){
        case CLEAR: return semanticParseCLEAR();
        case CROSS: return semanticParseCROSS();
        case CROSSTRANS: return semanticParseCROSSTRANSPOSE();
        case DISTINCT: return semanticParseDISTINCT();
        case EXPORT: return semanticParseEXPORT();
        case EXPORTM: return semanticParseEXPORTMATRIX();
        case INDEX: return semanticParseINDEX();
        case JOIN: return semanticParseJOIN();
        case NJOIN: return semanticParseNESTEDJOIN();
        case PHJOIN: return semanticParsePARTHASHJOIN();
        case GROUPBY: return semanticParseGROUPBY();
        case LIST: return semanticParseLIST();
        case LOAD: return semanticParseLOAD();
        case LOADM: return semanticParseLOADMATRIX();
        case PRINT: return semanticParsePRINT();
        case PRINTM: return semanticParsePRINTMATRIX();
        case PROJECTION: return semanticParsePROJECTION();
        case RENAME: return semanticParseRENAME();
        case SELECTION: return semanticParseSELECTION();
        case SORT: return semanticParseSORT();
        case SOURCE: return semanticParseSOURCE();
        default: cout<<"SEMANTIC ERROR"<<endl;
    }

    return false;
}