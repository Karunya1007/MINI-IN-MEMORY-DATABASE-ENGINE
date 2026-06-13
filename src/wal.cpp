#include "wal.h"
#include "database.h"
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;

WalLogger::WalLogger(const string& filename): filename_(filename){}

bool WalLogger::appendSet(const string& key, const string& value, long long timeToLiveSeconds)
{
    // writing to the file, called by the commit operation of save function in database.cpp
    ofstream fout(filename_, ios::app);
    if(!fout)return false;
    fout << "SET" << ' ' << key << ' ' << quoted(value) << ' ' << timeToLiveSeconds << '\n';
    return true;
}

bool WalLogger::appendDel(const string& key)
{
    ofstream fout(filename_, ios::app);
    if(!fout)return false;
    fout << "DEL" << ' ' << key << '\n';
    return true;
}

bool WalLogger::clear()
{
    ofstream fout(filename_, ios::trunc);
    return (bool)fout;
}

bool WalLogger::replay(DataBase& db)
{
    ifstream fin(filename_);
    if(!fin)return true;
    string line;
    while(getline(fin, line))
    {
        // we pushback each operation in the wal.log to the operations_ vector
        // so later whenever commit happens all those will be saved into the snapshot
        if(line.empty())continue;
        stringstream ss(line);
        string command;
        ss >> command;
        if(command == "SET")
        {
            string key, value;
            long long timeToLiveSeconds = -1;
            if(!(ss >> key))continue;
            if(!(ss >> quoted(value)))continue;
            if(!(ss >> timeToLiveSeconds))timeToLiveSeconds = -1;
            db.set(key, value, timeToLiveSeconds);
        }
        else if(command == "DEL")
        {
            string key;
            if(!(ss >> key))continue;
            db.del(key);
        }
    }
    return true;
}
