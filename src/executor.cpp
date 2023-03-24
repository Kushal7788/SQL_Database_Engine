#include"global.h"

void executeCommand(){

    switch(parsedQuery.queryType){
        case CLEAR: executeCLEAR(); break;
        case CROSS: executeCROSS(); break;
        case CROSSTRANS: executeCROSSTRANSPOSE(); break;
        case DISTINCT: executeDISTINCT(); break;
        case EXPORT: executeEXPORT(); break;
        case EXPORTM: executeEXPORTMATRIX(); break;
        case INDEX: executeINDEX(); break;
        case JOIN: executeJOIN(); break;
        case NJOIN: executeNESTEDJOIN(); break;
        case PHJOIN: executePARTHASHJOIN(); break;
        case GROUPBY: executeGROUPBY(); break;
        case LIST: executeLIST(); break;
        case LOAD: executeLOAD(); break;
        case LOADM: executeLOADMATRIX(); break;
        case PRINT: executePRINT(); break;
        case PRINTM: executePRINTMATRIX(); break;
        case PROJECTION: executePROJECTION(); break;
        case RENAME: executeRENAME(); break;
        case SELECTION: executeSELECTION(); break;
        case SORT: executeSORT(); break;
        case SOURCE: executeSOURCE(); break;
        default: cout<<"PARSING ERROR"<<endl;
    }

    return;
}

void printRowCount(int rowCount){
    cout<<"\n\nRow Count: "<<rowCount<<endl;
    return;
}