//===------ DataUtility.cpp - A utility class used for storing data   ----===//
//
//   EH-Miner: Mining Error-Handling Bugs without Error Specification Input
//
// Author: Zhouyang Jia, PhD Candidate
// Affiliation: School of Computer Science, National University of Defense Technology
// Email: jiazhouyang@nudt.edu.cn
//
//===----------------------------------------------------------------------===//
//
// This file implements the utility classes used for storing data.
//
//===----------------------------------------------------------------------===//

#include "DataUtility.h"

//===----------------------------------------------------------------------===//
//
//                     ConfigData Class
//
//===----------------------------------------------------------------------===//
// This class stores the data got from config file, including the domain
// information and projects in each domain. We use two static members to
// store the information so that it can be used all around our project.
//===----------------------------------------------------------------------===//

// The memebers are static, and used to store the domain and project name in config file
vector<string> ConfigData::domainName;
vector<vector<string>> ConfigData::projectName;

// Add a domain name from config file
void ConfigData::addDomainName(string name){
    domainName.push_back(name);
    vector<string> names;
    projectName.push_back(names);
}

// Add a project in domain numbered domainIndex
void ConfigData::addProjectName(unsigned int domainIndex, string name){
    projectName[domainIndex].push_back(name);
}

// Get the 1-dim domain name
vector<string> ConfigData::getDomainName(){
    return domainName;
}

// Get the 2-dim project name
vector<vector<string>> ConfigData::getProjectName(){
    return projectName;
}

// Print the static members
void ConfigData::printName(){
    for(unsigned i = 0; i < domainName.size(); i++){
        cout<<domainName[i]<<": ";
        for(unsigned j = 0; j < projectName[i].size(); j++){
            cout<<projectName[i][j];
            if(j != projectName[i].size()-1)
                cout<<", ";
            else
                cout<<"\n";
        }
    }
    return;
}

//===----------------------------------------------------------------------===//
//
//                     CallData Class
//
//===----------------------------------------------------------------------===//
// This class is designed to store the information of function calls, especially,
// branch-related information.
//===----------------------------------------------------------------------===//

ConfigData CallData::configData;
sqlite3* CallData::db;

// Open the SQLite database
void CallData::openDatabase(string file){
    if(file.empty()){
        fprintf(stderr, "The database file is empty!\n");
        exit(1);
    }
    
    int rc = sqlite3_open(file.c_str(), &db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    
    sqlite3_busy_timeout(db, 10*1000); // wait for 10s when the database is locked
}

// Close the SQLite database
void CallData::closeDatabase(){
    sqlite3_close(db);
}

// Get the SQLite database
sqlite3* CallData::getDatabase(){
    return db;
}

string& replace_all_distinct(string& str,const string& old_value, const string& new_value)
{
    string::size_type pos = 0;
    while ( (pos = str.find(old_value, pos)) != string::npos ) {
        str.replace( pos, old_value.size(), new_value );
        pos+=new_value.size();
    }
    return str;
}

// Callback function to get the selected data in  SQLite
static int cb_get_info(void *data, int argc, char **argv, char **azColName){
    
    pair<int, vector<string>>* rowdata = (pair<int, vector<string>>*) data;
    
    // Multiple rows have the same function name, which should not happen.
    if(rowdata->first != 0){
        fprintf(stderr, "Multiple rows have the same function name\n");
        return SQLITE_ERROR;
    }
    
    // Print the row
    //int i;
    //printf("Selected: ");
    //for(i=0; i<argc; i++){
    //    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    //}
    //printf("\n");
    
    // Get the values
    vector<string> values;
    for(int i = 0; i < argc; i++){
        string value = argv[i];
        values.push_back(value);
    }
    
    // Store the ID and values
    *rowdata = make_pair(atoi(argv[0]), values);
    return SQLITE_OK;
}

// Add a branch call
void CallData::addBranchCall(BranchInfo branchInfo){
    
    // Get the domain name and project name from given path
    pair<string, string> mDomProName = getDomainProjectName(branchInfo.callID);
    string domainName = mDomProName.first;
    string projectName = mDomProName.second;
    if(domainName.empty() || projectName.empty()){
        return;
    }
    
    // Prepare the sql stmt to create the table
    int rc;
    char *zErrMsg = 0;
    string stmt = "create table if not exists branch_call (ID integer primary key autoincrement, DomainName text, ProjectName text, CallName text, CallDefLoc text, CallID text, CallStr text, CallReturn text, CallArgVec text, CallArgNum text, ExprNodeVec text, ExprNodeNum text, ExprStrVec text, PathNumberVec text, CaseLabelVec text, BranchLevel text, LogName text, LogDefLoc text, LogID text, LogStr text, LogArgVec text, LogArgNum text, LogRetType text, LogArgTypeVec text, LogArgTypeNum text)";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
        rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS call1_index ON branch_call(CallName, CallDefLoc)", 0, 0, &zErrMsg);
    
    // Prepare the sql stmt to insert new entry
    string callReturnVecStr;
    for(unsigned i = 0; i < branchInfo.callReturnVec.size(); i++){
        callReturnVecStr += branchInfo.callReturnVec[i];
        if(i != branchInfo.callReturnVec.size() - 1)
            callReturnVecStr += "#-_-#";
    }
    if(callReturnVecStr == "")
        callReturnVecStr = "-";
    
    string callArgVecStr;
    for(unsigned i = 0; i < branchInfo.callArgVec.size(); i++){
        callArgVecStr += branchInfo.callArgVec[i];
        if(i != branchInfo.callArgVec.size() - 1)
            callArgVecStr += "#-_-#";
    }
    if(callArgVecStr == "")
        callArgVecStr = "-";
    
    string exprNodeVecStr;
    for(unsigned i = 0; i < branchInfo.exprNodeVec.size(); i++){
        exprNodeVecStr += branchInfo.exprNodeVec[i];
        if(i != branchInfo.exprNodeVec.size() - 1)
            exprNodeVecStr += "#-_-#";
    }
    if(exprNodeVecStr == "")
        exprNodeVecStr = "-";
    
    string logArgVecStr;
    for(unsigned i = 0; i < branchInfo.logArgVec.size(); i++){
        logArgVecStr += branchInfo.logArgVec[i];
        if(i != branchInfo.logArgVec.size() - 1)
            logArgVecStr += "#-_-#";
    }
    if(logArgVecStr == "")
        logArgVecStr = "-";
    
    string logArgTypeVecStr;
    for(unsigned i = 0; i < branchInfo.logArgTypeVec.size(); i++){
        logArgTypeVecStr += branchInfo.logArgTypeVec[i];
        if(i != branchInfo.logArgVec.size() - 1)
            logArgTypeVecStr += "#-_-#";
    }
    if(logArgTypeVecStr == "")
        logArgTypeVecStr = "-";
    
    string exprStrVecStr;
    for(unsigned i = 0; i < branchInfo.exprStrVec.size(); i++){
        exprStrVecStr += branchInfo.exprStrVec[i];
        if(i != branchInfo.exprStrVec.size() - 1)
            exprStrVecStr += "#-_-#";
    }
    if(exprStrVecStr == "")
        exprStrVecStr = "-";
    
    string pathNumberVecStr;
    for(unsigned i = 0; i < branchInfo.pathNumberVec.size(); i++){
        char pathNumber[10];
        sprintf(pathNumber, "%d", branchInfo.pathNumberVec[i]);
        pathNumberVecStr += pathNumber;
        if(i != branchInfo.pathNumberVec.size() - 1)
            pathNumberVecStr += "#-_-#";
    }
    if(pathNumberVecStr == "")
        pathNumberVecStr = "-";
    
    string caseLabelVecStr;
    for(unsigned i = 0; i < branchInfo.caseLabelVec.size(); i++){
        caseLabelVecStr += branchInfo.caseLabelVec[i];
        if(i != branchInfo.caseLabelVec.size() - 1)
            caseLabelVecStr += "#-_-#";
    }
    if(caseLabelVecStr == "")
        caseLabelVecStr = "-";
    
    char callArgNumStr[10];
    char exprNodeNumStr[10];
    char branchLevelStr[10];
    char logArgNumStr[10];
    char logArgTypeNumStr[10];
    sprintf(callArgNumStr, "%lu", branchInfo.callArgVec.size());
    sprintf(exprNodeNumStr, "%lu", branchInfo.exprNodeVec.size());
    sprintf(branchLevelStr, "%lu", branchInfo.exprStrVec.size());
    sprintf(logArgNumStr, "%lu", branchInfo.logArgVec.size());
    sprintf(logArgTypeNumStr, "%lu", branchInfo.logArgTypeVec.size());
    
    // Replace "'" by "''" to make sqlite happy
    branchInfo.callStr = replace_all_distinct(branchInfo.callStr, "'", "''");
    callReturnVecStr = replace_all_distinct(callReturnVecStr, "'", "''");
    callArgVecStr = replace_all_distinct(callArgVecStr, "'", "''");
    exprNodeVecStr = replace_all_distinct(exprNodeVecStr, "'", "''");
    exprStrVecStr = replace_all_distinct(exprStrVecStr, "'", "''");
    caseLabelVecStr = replace_all_distinct(caseLabelVecStr, "'", "''");
    branchInfo.logStr = replace_all_distinct(branchInfo.logStr, "'", "''");
    logArgVecStr = replace_all_distinct(logArgVecStr, "'", "''");
    
    stmt = "insert into branch_call (DomainName, ProjectName, CallName, CallDefLoc, CallID, CallStr, CallReturn, CallArgVec, CallArgNum, ExprNodeVec, ExprNodeNum, ExprStrVec, PathNumberVec, CaseLabelVec, BranchLevel, LogName, LogDefLoc, LogID, LogStr, LogArgVec, LogArgNum, LogRetType, LogArgTypeVec, LogArgTypeNum) values ('" + domainName + "', '" + projectName + "', '" + branchInfo.callName + "', '" + branchInfo.callDefLoc + "', '" + branchInfo.callID + "', '" + branchInfo.callStr + "', '" + callReturnVecStr + "', '" + callArgVecStr + "', '" + callArgNumStr + "', '" + exprNodeVecStr + "', '" + exprNodeNumStr + "', '" + exprStrVecStr + "', '" + pathNumberVecStr + "', '" + caseLabelVecStr + "', '" + branchLevelStr + "', '" + branchInfo.logName + "', '" + branchInfo.logDefLoc + "', '" + branchInfo.logID + "', '" + branchInfo.logStr + "', '" + logArgVecStr + "', '" + logArgNumStr + "', '" + branchInfo.logRetType + "', '" + logArgTypeVecStr + "', '" + logArgTypeNumStr +"')";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    //cerr<<stmt<<endl;     // for debug
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    return;
}

// Add a pre-branch call
void CallData::addPrebranchCall(string callName, string callLocFullPath, string callDefFullPath, string logName, string logDefFullPath){
    
    // Get the domain name and project name from given path
    pair<string, string> mDomProName = getDomainProjectName(callLocFullPath);
    string domainName = mDomProName.first;
    string projectName = mDomProName.second;
    if(domainName.empty() || projectName.empty()){
        return;
    }
    
    // Prepare the sql stmt to create the table
    int rc;
    char *zErrMsg = 0;
    string stmt = "create table if not exists prebranch_call (ID integer primary key autoincrement, CallName text, CallDefLoc text, DomainName text, ProjectName text, LogName text, LogDefLoc text, NumLogTime integer)";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
        rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS call2_index ON prebranch_call(CallName, CallDefLoc)", 0, 0, &zErrMsg);
    
    // Check whether the function is called the first time
    int ID = 0;
    vector<string> values;
    values.clear();
    // Store ID and values for the each row
    pair<int, vector<string>> rowdata = make_pair(ID, values);
    stmt="select * from prebranch_call where LogName = '" + logName + "' and LogDefLoc = '" + logDefFullPath + "' and CallName = '" + callName + "' and CallDefLoc = '" + callDefFullPath + "' and DomainName = '" + domainName + "' and ProjectName = '" + projectName + "'";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), cb_get_info, &rowdata, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    // The function is called the first time
    if(rowdata.first == 0){
        
        // Prepare the sql stmt to insert new entry
        stmt = "insert into prebranch_call (CallName, CallDefLoc, DomainName, ProjectName, LogName, LogDefLoc, NumLogTime) values ('" + callName + "', '" + callDefFullPath + "', '" + domainName + "', '" + projectName + "', '" + logName + "', '" + logDefFullPath + "', 1)";
        if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            cerr<<stmt<<endl;
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    else{
        // Prepare the sql stmt to update the entry
        //get old value
        int numLogTime =  atoi(rowdata.second[7].c_str());
        //get new value
        numLogTime++;
        //convert from int to char*
        char numLogTimeStr[10];
        sprintf(numLogTimeStr, "%d", numLogTime);
        string numLogTimeString = numLogTimeStr;
        //combine whole sql statement
        stmt = "update prebranch_call set NumLogTime = " + numLogTimeString + " where LogName = '" + logName + "' and LogDefLoc = '" + logDefFullPath + "' and CallName = '" + callName + "' and CallDefLoc = '" + callDefFullPath + "' and DomainName = '" + domainName + "' and ProjectName = '" + projectName + "'";
        if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
        //execute the update stmt
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            cerr<<stmt<<endl;
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    return;
}

// Add a post-branch call
void CallData::addPostbranchCall(string callName, string callLocFullPath, string callDefFullPath, string logName, string logDefFullPath){
    
    // Get the domain name and project name from given path
    pair<string, string> mDomProName = getDomainProjectName(callLocFullPath);
    string domainName = mDomProName.first;
    string projectName = mDomProName.second;
    if(domainName.empty() || projectName.empty()){
        return;
    }
    
    // Prepare the sql stmt to create the table
    int rc;
    char *zErrMsg = 0;
    string stmt = "create table if not exists postbranch_call (ID integer primary key autoincrement, LogName text, LogDefLoc text, DomainName text, ProjectName text, PrebranchCall text, NumPrebranchCall integer, NumPostbranchCall integer)";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
        rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS log_index ON postbranch_call(LogName, LogDefLoc)", 0, 0, &zErrMsg);

    // Check whether the function is called the first time
    int ID = 0;
    vector<string> values;
    values.clear();
    // Store ID and values for the certain row
    pair<int, vector<string>> rowdata = make_pair(ID, values);
    stmt="select * from postbranch_call where LogName = '" + logName + "' and LogDefLoc = '" + logDefFullPath + "' and DomainName = '" + domainName + "' and ProjectName = '" + projectName + "'";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), cb_get_info, &rowdata, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    // The function is called the first time
    if(rowdata.first == 0){

        // Prepare the sql stmt to insert new entry
        stmt = "insert into postbranch_call (LogName, LogDefLoc, DomainName, ProjectName, PrebranchCall, NumPrebranchCall, NumPostbranchCall) values ('" + logName + "', '" + logDefFullPath + "', '" + domainName + "', '" + projectName + "', '#" + callName + "#', 1, 1)";
        if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            cerr<<stmt<<endl;
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    else{
        // Prepare the sql stmt to update the entry
        //get old value
        string prebranchCall = rowdata.second[5];
        int numPrebranchCall =  atoi(rowdata.second[6].c_str());
        int numPostbranchCall =  atoi(rowdata.second[7].c_str());
        //get new value
        if(prebranchCall.find("#" + callName + "#") == string::npos){
            prebranchCall += callName + "#";
            numPrebranchCall++;
        }
        numPostbranchCall++;
        //convert from int to char*
        char numPrebranchCallStr[10];
        char numPostbranchCallStr[10];
        sprintf(numPrebranchCallStr, "%d", numPrebranchCall);
        sprintf(numPostbranchCallStr, "%d", numPostbranchCall);
        //combine whole sql statement
        stmt = "update postbranch_call set PrebranchCall = '" + prebranchCall + "', NumPrebranchCall = " + numPrebranchCallStr + ", NumPostbranchCall = " + numPostbranchCallStr + " where LogName = '" + logName + "' and LogDefLoc = '" + logDefFullPath + "' and DomainName = '" + domainName + "' and ProjectName = '" + projectName + "'";
        if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
        //execute the update stmt
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            cerr<<stmt<<endl;
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    return;
}

// Add an edge of call graph
void CallData::addCallGraph(string funcName, string funcDefFullPath, string callName, string callDefFullPath, string callLocFullPath, unsigned funcSize){
    
    // Get the domain name and project name from given path
    pair<string, string> mDomProName = getDomainProjectName(callLocFullPath);
    string domainName = mDomProName.first;
    string projectName = mDomProName.second;
    if(domainName.empty() || projectName.empty()){
        return;
    }
    
    // Prepare the sql stmt to create the table
    int rc;
    char *zErrMsg = 0;
    string stmt = "create table if not exists call_graph (ID integer primary key autoincrement, FuncName text, FuncDefLoc text, FuncSize integer, DomainName text, ProjectName text, CallName text, CallDefLoc text)";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
        rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS func_index ON call_graph(FuncName, FuncDefLoc)", 0, 0, &zErrMsg);
        
    // Prepare the sql stmt to insert new entry
    ostringstream oss;
    oss << funcSize;
    stmt = "insert into call_graph (FuncName, FuncDefLoc, FuncSize, DomainName, ProjectName, CallName, CallDefLoc) values ('" + funcName + "', '" + funcDefFullPath + "', '" + oss.str() + "', '" + domainName + "', '" + projectName + "', '" + callName + "', '" + callDefFullPath +"')";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return;
}

// Add a function call and update call_statistic
void CallData::addFunctionCall(string callName, string callLocFullPath, string callDefFullPath, string callStr){
    
    // Get the domain name and project name from given path
    pair<string, string> mDomProName = getDomainProjectName(callLocFullPath);
    string domainName = mDomProName.first;
    string projectName = mDomProName.second;
    if(domainName.empty() || projectName.empty()){
        return;
    }
    
    
    // Prepare the sql stmt to create the table
    int rc;
    char *zErrMsg = 0;
    string stmt = "create table if not exists function_call (ID integer primary key autoincrement, CallName text, CallDefLoc text, DomainName text, ProjectName text, CallID text, CallStr text)";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
        rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS call4_index ON function_call(CallName, CallDefLoc)", 0, 0, &zErrMsg);
    
    callStr = replace_all_distinct(callStr, "'", "''");
    // Prepare the sql stmt to insert new entry
    stmt = "insert into function_call (CallName, CallDefLoc, DomainName, ProjectName, CallID, CallStr) values ('" + callName + "', '" + callDefFullPath + "', '" + domainName + "', '" + projectName + "', '" + callLocFullPath + "', '" + callStr + "')";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    // Prepare the sql stmt to create the table
    stmt = "create table if not exists call_statistic (ID integer primary key autoincrement, CallName text, CallDefLoc text, DomainName text, ProjectName text, CallNumber integer)";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
        rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS call3_index ON call_statistic(CallName, CallDefLoc)", 0, 0, &zErrMsg);
    
    // Check whether the function is called the first time
    int ID = 0;
    vector<string> values;
    values.clear();
    // Store ID and values for the certain row
    pair<int, vector<string>> rowdata = make_pair(ID, values);
    stmt="select * from call_statistic where CallName = '" + callName + "' and CallDefLoc = '" + callDefFullPath + "' and DomainName = '" + domainName + "' and ProjectName = '" + projectName + "'";
    if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
    rc = sqlite3_exec(db, stmt.c_str(), cb_get_info, &rowdata, &zErrMsg);
    if(rc!=SQLITE_OK){
        cerr<<stmt<<endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    // The function is called the first time
    if(rowdata.first == 0){
        
        // Prepare the sql stmt to insert new entry
        stmt = "insert into call_statistic (CallName, CallDefLoc, DomainName, ProjectName, CallNumber) values ('" + callName + "', '" + callDefFullPath + "', '" + domainName + "', '" + projectName + "' , 1)";
        if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            cerr<<stmt<<endl;
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    else{
        // Prepare the sql stmt to update the entry
        //get old value
        int callnumber =  atoi(rowdata.second[5].c_str());
        //get new value
        callnumber++;
        //convert from int to char*
        char callnumberStr[10];
        sprintf(callnumberStr, "%d", callnumber);
        string callnumberString = callnumberStr;
        stmt = "update call_statistic set CallNumber = " + callnumberString + " where CallName = '" + callName + "' and CallDefLoc = '" + callDefFullPath + "' and DomainName = '" + domainName + "' and ProjectName = '" + projectName + "'";
        //execute the update stmt
        if(OUTPUT_SQL_STMT)cerr<<stmt<<endl;
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            cerr<<stmt<<endl;
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    return;
}

// Get the domain and project name from the full path of the file
pair<string, string> CallData::getDomainProjectName(string callLocation){
    
    // Get the domain and project name
    vector<string> mDomainName = configData.getDomainName();
    vector<vector<string>> mProjectName = configData.getProjectName();
    
    // For each domain
    for(unsigned i = 0; i < mDomainName.size(); i++){
        
        // The full path contains the domain name
        if(callLocation.find("/"+mDomainName[i]+"/") != string::npos){
            
            // For each project
            for(unsigned j = 0; j < mProjectName[i].size(); j++){
                
                // The full path contains the project name
                if(callLocation.find("/"+mProjectName[i][j]+"/") != string::npos){
                    
                    // Make the pair of doamin name and project name, and return
                    return make_pair(mDomainName[i], mProjectName[i][j]);
                }
            }
        }
    }
    
    // Return an empty pair
    return make_pair("", "");
}
