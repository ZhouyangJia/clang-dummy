//===------ DataUtility.cpp - A utility class used for storing data   ----===//
//
//                     Cross Project Checking
//
// Author: Zhouyang Jia, PhD Candidate
// Affiliation: School of Computer Science, National University of Defense Technology
// Email: jiazhouyang@nudt.edu.cn
//
//===----------------------------------------------------------------------===//
//
// This file implements a utility class used for storing data.
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
//                     CallInfo Struct
//
//===----------------------------------------------------------------------===//
// In this struct, we store all the needed information for all the functions
// called in all projects. Each instance handles one function, including the
// function name, call times, def and use location, etc.
//===----------------------------------------------------------------------===//

// Init for CallInfo, each instance of CallInfor stores infomation for one call
// The meaning of each member can be found in DataUtility.h
CallInfo::CallInfo(){
    callName = "";
    numDomain = 0;
    numProject = 0;
    numCallTotal = 0;
    memset(numCallInDomain, 0, MAX_PROJECT*sizeof(int));
    memset(numCallInProject, 0, MAX_PROJECT*sizeof(int));
    defLocation = "";
    multiDef = false;
    outProjectDef = false;
}

// Print the call infomation
void CallInfo::print(int domainNumber, int projectNumber){
    
    // callName, defLocation, outProjectDef, multiDef, numDomain, numProject, numCallTotal,
    cout<<callName<<", ";
    cout<<defLocation<<", "<<outProjectDef<<", "<<multiDef<<", ";
    cout<<numDomain<<", "<<numProject<<", "<<numCallTotal<<", ";
    
    for(int i = 0; i < domainNumber; i++){
        cout<<numCallInDomain[i]<<", ";
    }
    
    for(int i = 0; i < projectNumber; i++){
        cout<<numCallInProject[i];
        if(i == projectNumber - 1)
            cout<<endl;
        else
            cout<<", ";
    }
    return;
}

//===----------------------------------------------------------------------===//
//
//                     CallData Class
//
//===----------------------------------------------------------------------===//
// This class is designed to store the call infomation of all functions. Since
// each FroentAction instance may have fucntion calls to store, all the members
// are static so that they will be shared by all FroentAction instances.
//===----------------------------------------------------------------------===//

// The memebers are static, and used to store all the call information
// The meaning of each member can be found in DataUtility.h
ConfigData CallData::configData;
int CallData::totalIndex;
map<string, int> CallData::call2index;
vector<CallInfo> CallData::callInfoVec;
string CallData::databaseName;

// Set the name of SQLite database
void CallData::setDatabase(string name){
    databaseName = name;
}

// Print all the call infomation
void CallData::print(){
    
    if(totalIndex == 0)
        return;
    
    cout<<"Call ID, Call Name, Def Location, Has Out Def, Is Multi Def, Num Domain, Num Project, Num Call Total";
    
    int domainNumber = 0;
    int projectNumber = 0;
    
    vector<string> domainName = configData.getDomainName();
    domainNumber = domainName.size();
    for(unsigned i = 0; i < domainName.size(); i++){
        cout<<", domain:"<<domainName[i];
    }
    
    
    vector<vector<string>> projectName = configData.getProjectName();
    for(unsigned i = 0; i < projectName.size(); i++){
        projectNumber += projectName[i].size();
        for(unsigned j = 0; j < projectName[i].size(); j++){
            cout<<", "<<domainName[i]<<":"<<projectName[i][j];
        }
    }
    cout<<endl;
    
    for(int i = 1; i <= totalIndex; i++){
        cout<<i<<", ";
        callInfoVec[i-1].print(domainNumber, projectNumber);
    }
    return;
}

#ifdef SQLITE
// Callback of SQLite
static int callback(void *data, int argc, char **argv, char **azColName){
    
    pair<int, vector<string>>* rowdata = (pair<int, vector<string>>*) data;
    
    // Multiple rows have the same function name, which should not happen.
    if(rowdata->first != 0){
        fprintf(stderr, "Multiple rows have the same function name\n");
        return SQLITE_ERROR;
    }
    
    // Print the row
    //int i;
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
#endif

// Add a call expression
void CallData::addCallExpression(string callName, string callLocation, string defLocation){
    
    // Get the domain and project name
    vector<string> mDomainName = configData.getDomainName();
    vector<vector<string>> mProjectName = configData.getProjectName();

    // Get the domain name and project name of given path
    pair<string, string> mDomProName = getDomainProjectName(callLocation);
    string domainName = mDomProName.first;
    string projectName = mDomProName.second;
    if(domainName.empty() || projectName.empty()){
        // Try to get the names from defLocation
        mDomProName = getDomainProjectName(defLocation);
        domainName = mDomProName.first;
        projectName = mDomProName.second;
        
        if(domainName.empty() || projectName.empty()){
            cerr<<"Cannot find domain or project name for "<<callName<<":\n";
            cerr<<"\tcall@"<<callLocation<<"&"<<"def@"<<defLocation<<".\n";
            return;
        }
    }
    
    // Get the domain ID and project ID of given path
    pair<int, int> mDomProID = getDomainProjectID(domainName, projectName);
    int domainID = mDomProID.first;
    int projectID = mDomProID.second;
    if(domainID == -1 || projectID == -1){
        cerr<<"Cannot find the ID of the given domain and project name: "<<domainName<<" and "<<projectName<<"!\n";
        return;
    }
    if(domainID >= MAX_PROJECT || projectID >= MAX_PROJECT){
        cerr<<"Exceed the project limitation: "<<MAX_PROJECT<<".\n";
        return;
    }
    
    // The function is called the first time
    if(call2index[callName] == 0){
        
        // Update and assign the index
        totalIndex++;
        call2index[callName] = totalIndex;
        
        // Declare the CallInfo instance, and init
        CallInfo callInfo;
        
        callInfo.callName = callName;
        callInfo.numProject++;
        callInfo.numDomain++;
        callInfo.numCallTotal++;
        callInfo.numCallInProject[projectID]++;
        callInfo.numCallInDomain[domainID]++;
        callInfo.defLocation = defLocation;
        callInfo.multiDef = false;
        callInfo.outProjectDef = isOutProjectDef(defLocation, domainName, projectName);
        
        // Push the instance to callInfoVec
        callInfoVec.push_back(callInfo);
    }
    // The function has been called
    else{
        // Get the index (call2index is starting from 1)
        int curIndex = call2index[callName] - 1;
        
        // Update the infomation
        callInfoVec[curIndex].numCallTotal++;
        if(callInfoVec[curIndex].numCallInProject[projectID] == 0)
            callInfoVec[curIndex].numProject++;
        if(callInfoVec[curIndex].numCallInDomain[domainID] == 0)
            callInfoVec[curIndex].numDomain++;
        callInfoVec[curIndex].numCallInProject[projectID]++;
        callInfoVec[curIndex].numCallInDomain[domainID]++;
        if(callInfoVec[curIndex].defLocation.find(defLocation) == string::npos){
            callInfoVec[curIndex].multiDef = true;
            callInfoVec[curIndex].defLocation += "#";
            callInfoVec[curIndex].defLocation += defLocation;
        }
        if(isOutProjectDef(defLocation, domainName, projectName))
            callInfoVec[curIndex].outProjectDef = true;
    }
    
    
#ifdef SQLITE
    // If compile the tool with option -DSQLITE=ON, we can use database to store the data,
    // of which name is specified by -database-file=<database_file_name>.
    if(databaseName.empty())
        return;
    
    // Init and open database
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_open(databaseName.c_str(), &db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    // Prepare the sql stmt to create the table
    string stmt = "create table if not exists function_info (\
    ID integer primary key autoincrement, \
    CallName text, DefLoc text, HasOutDef integer, IsMulDef integer, \
    NumDomain integer, NumProject integer, NumCallTotal integer";
    for(unsigned i = 0; i < mDomainName.size(); i++){
        stmt+=", " + mDomainName[i] + " integer";
    }
    for(unsigned i = 0; i < mDomainName.size(); i++){
        for(unsigned j = 0; j < mProjectName[i].size(); j++){
            stmt+=", " + mProjectName[i][j] + " integer";
        }
    }
    stmt+=")";
    rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
    if(rc!=SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    // Check whether the function is called the first time
    int ID = 0;
    vector<string> values;
    values.clear();
    // Store ID and values for the certain row
    pair<int, vector<string>> rowdata = make_pair(ID, values);
    stmt="select * from function_info where CallName = '" + callName + "'";
    rc = sqlite3_exec(db, stmt.c_str(), callback, &rowdata, &zErrMsg);
    if(rc!=SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    // The function is called the first time
    if(rowdata.first == 0){
        
        // Prepare the sql stmt to insert new entry
        stmt = "insert into function_info (CallName, DefLoc, HasOutDef, IsMulDef, NumDomain, NumProject, NumCallTotal";
        for(unsigned i = 0; i < mDomainName.size(); i++){
            stmt+=", " + mDomainName[i];
        }
        for(unsigned i = 0; i < mDomainName.size(); i++){
            for(unsigned j = 0; j < mProjectName[i].size(); j++){
                stmt+=", " + mProjectName[i][j];
            }
        }
        string isOutDef;
        if(isOutProjectDef(defLocation, domainName, projectName))
            isOutDef = "1";
        else
            isOutDef = "0";
        stmt+= ") values ('" + callName + "', '" + defLocation + "', " + isOutDef + ", 0, 1, 1, 1";
        for(unsigned i = 0; i < mDomainName.size(); i++){
            if(domainName.find(mDomainName[i]) != string::npos)
                stmt+=", 1";
            else
                stmt+=", 0";
        }
        for(unsigned i = 0; i < mDomainName.size(); i++){
            for(unsigned j = 0; j < mProjectName[i].size(); j++){
                if(projectName.find(mProjectName[i][j]) != string::npos)
                    stmt+=", 1";
                else
                    stmt+=", 0";
            }
        }
        stmt+=")";
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    else{
        //for(unsigned i = 0; i < rowdata.second.size(); i++){
        //    cout<<rowdata.second[i]<<endl;
        //}
        
        // Prepare the sql stmt to update the entry
        //get old value
        string defloc = rowdata.second[2];
        string outdef = rowdata.second[3];
        string multidef = rowdata.second[4];
        int numdomain =  atoi(rowdata.second[5].c_str());
        int numproject =  atoi(rowdata.second[6].c_str());
        int numcalltotal = atoi(rowdata.second[7].c_str());
        int numcurdomain = atoi(rowdata.second[8+domainID].c_str());
        int numcurproject = atoi(rowdata.second[8+mDomainName.size()+projectID].c_str());
        //get new value
        if(isOutProjectDef(defLocation, domainName, projectName))
            outdef = "1";
        if(defloc.find(defLocation) == string::npos){
            multidef = "1";
            defloc += "#" + defLocation;
        }
        if(numcurdomain == 0)
            numdomain++;
        if(numcurproject == 0)
            numproject++;
        numcalltotal++;
        numcurdomain++;
        numcurproject++;
        //convert from int to char*
        char numdomainStr[10];
        char numprojectStr[10];
        char numcalltotalStr[10];
        char numcurdomainStr[10];
        char numcurprojectStr[10];
        sprintf(numdomainStr, "%d", numdomain);
        sprintf(numprojectStr, "%d", numproject);
        sprintf(numcalltotalStr, "%d", numcalltotal);
        sprintf(numcurdomainStr, "%d", numcurdomain);
        sprintf(numcurprojectStr, "%d", numcurproject);
        //get index of domain and project in mDomainName and mProjectName
        int domainindex = domainID;
        int projectindex = 0;
        for(unsigned i = 0; i < mProjectName[domainindex].size(); i++){
            if(mProjectName[domainindex][i] == projectName){
                projectindex = i;
                break;
            }
        }
        //combine whole sql statement
        stmt = "update function_info set DefLoc = '" + defloc + "', HasOutDef = " + outdef + ", IsMulDef = " + multidef + \
                ", NumDomain = " + numdomainStr + ", NumProject = " + numprojectStr + ", NumCallTotal = " + numcalltotalStr + \
                ", " + mDomainName[domainindex] + " = " + numcurdomainStr + ", " + mProjectName[domainindex][projectindex] + " = " + \
                numcurprojectStr + " where CallName = '" + callName + "'";
        //execute the update stmt
        rc = sqlite3_exec(db, stmt.c_str(), 0, 0, &zErrMsg);
        if(rc!=SQLITE_OK){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    
    // Close the database
    sqlite3_close(db);
#endif
  
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
        if(callLocation.find("/"+mDomainName[i]) != string::npos){
            
            // For each project
            for(unsigned j = 0; j < mProjectName[i].size(); j++){
                
                // The full path contains the project name
                if(callLocation.find("/"+mProjectName[i][j]) != string::npos){
                    
                    // Make the pair of doamin name and project name, and return
                    return make_pair(mDomainName[i], mProjectName[i][j]);
                }
            }
        }
    }
    
    // Return an empty pair
    return make_pair("", "");
}

// Get the project ID
pair<int, int> CallData::getDomainProjectID(string domainName, string projectName){
    
    int domID = 0;
    int projID = 0;
    
    // Get the domain and project name
    vector<string> mDomainName = configData.getDomainName();
    vector<vector<string>> mProjectName = configData.getProjectName();
    
    // For each domain
    for(unsigned i = 0; i < mDomainName.size(); i++){
        
        // The domain is equal to the parameter domainName
        if(domainName == mDomainName[i]){
            domID = i;
            // For each project
            for(unsigned j = 0; j < mProjectName[i].size(); j++){
                
                // The project is equal to the projectName
                if(projectName == mProjectName[i][j]){
                    
                    // Add the index of project in current domain
                    projID += j;
                    return make_pair(domID, projID);
                }
            }
        }
        else{
            // Add the number of projects in this domain
            projID += mProjectName[i].size();
        }
    }
    
    // Fail to find the corresponding domain and project, which is not supposed to happen.
    return make_pair(-1, -1);
}

// Is the definition locates out of the project, aka, is used the third part library
bool CallData::isOutProjectDef(string defLocation, string domainName, string projectName){
    
    if(defLocation.find("/"+domainName) != string::npos && defLocation.find("/"+projectName) != string::npos){
        return false;
    }
    return true;
}
