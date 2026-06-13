#pragma once
#include "wal.h"
#include "database.h"
#include <vector>
#include <string>

class Transaction
{
public:
    Transaction(DataBase& db, WalLogger* wallog);
    void set(const std::string& key, const std::string& value, long long timeToLiveSeconds = -1);
    void del(const std::string& key);
    bool commit();
    void rollBack();
    bool active() const;

private:
    DataBase& db_;
    WalLogger* wal_;
    std::vector<DataBase::Operation> operations_;
    // vector used for storing all transaction set-get operation 
    bool open_ = true;
};