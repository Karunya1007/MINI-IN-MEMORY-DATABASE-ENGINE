#include "database.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace std::chrono;

bool DataBase::isExpiredLocked(const Record &record) const
{
    return ((record.hasExpiry) && (system_clock::now() >= record.expiry));
}

void DataBase::eraseKeyLocked(const std::string &key)
{
    kv_.erase(key);
    ordered_.erase(key);
}

bool DataBase::setLocked(const std::string &key, const std::string &value, long long timeToLiveSeconds)
{
    Record record;
    record.value = value;
    if (timeToLiveSeconds >= 1)
    {
        record.hasExpiry = true;
        record.expiry = system_clock::now() + seconds(timeToLiveSeconds);
    }
    kv_[key] = record;
    ordered_[key] = value;
    return true;
}

bool DataBase::delLocked(const string &key)
{
    if (kv_.find(key) == kv_.end())
    {
        return false;
    }
    else
    {
        eraseKeyLocked(key);
        return true;
    }
}

bool DataBase::set(const string &key, const string &value, long long timeToLiveSeconds)
{
    lock_guard<mutex> lock(mtx_);
    return setLocked(key, value, timeToLiveSeconds);
}

bool DataBase::get(const string &key, string &valueOut)
{
    lock_guard<mutex> lock(mtx_);
    auto kv_It = kv_.find(key);
    if (kv_It == kv_.end())
        return false;
    if (isExpiredLocked(kv_It->second))
    {
        eraseKeyLocked(key);
        return false;
    }
    valueOut = (kv_It->second).value;
    return true;
}

bool DataBase::del(const string &key)
{
    lock_guard<mutex> lock(mtx_);
    return delLocked(key);
}

bool DataBase::exists(const string &key)
{
    lock_guard<mutex> lock(mtx_);
    auto kv_It = kv_.find(key);
    if (kv_It == kv_.end())
        return false;
    if (isExpiredLocked(kv_It->second))
    {
        eraseKeyLocked(key);
        return false;
    }
    return true;
}

vector<pair<string, string>> DataBase::scan()
{
    lock_guard<mutex> lock(mtx_);
    vector<pair<string, string>> out;
    for (auto ordered_It = ordered_.begin(); ordered_It != ordered_.end();)
    {
        auto kv_It = kv_.find(ordered_It->first);
        if (kv_It == kv_.end())
        {
            ordered_It = ordered_.erase(ordered_It);
            continue;
        }
        if (isExpiredLocked(kv_It->second))
        {
            string deadKey = kv_It->first;
            ordered_It++;
            eraseKeyLocked(deadKey);
            continue;
        }
        out.push_back(*ordered_It);
        ordered_It++;
    }
    return out;
}

vector<pair<string, string>> DataBase::rangeScan(const string &start, const string &end)
{
    lock_guard<mutex> lock(mtx_);
    vector<pair<string, string>> out;
    if (start > end)
        return out;
    auto ordered_It = ordered_.lower_bound(start);
    while (ordered_It != ordered_.end() && ordered_It->first <= end)
    {
        auto kv_It = kv_.find(ordered_It->first);
        if (kv_It != kv_.end() && !isExpiredLocked(kv_It->second))
        {
            out.push_back(*ordered_It);
        }
        ++ordered_It;
    }
    return out;
}

void DataBase::cleanUpExpired()
{
    lock_guard<mutex> lock(mtx_);
    vector<string> dead;
    dead.reserve(kv_.size());
    for (auto &[key, record] : kv_)
    {
        if (isExpiredLocked(record))
        {
            dead.push_back(key);
        }
    }
    for(auto& key: dead){
        eraseKeyLocked(key);
    }
    return;
}

bool DataBase::save(const string &filename)
{
    lock_guard<mutex> lock(mtx_);
    ofstream fout(filename);
    if (!fout)
        return false;
    for (auto &[key, record] : kv_)
    {
        if (isExpiredLocked(record))
        {
            continue;
        }
        long long expiryMilliSeconds = -1;
        if (record.hasExpiry)
        {
            expiryMilliSeconds = duration_cast<milliseconds>(record.expiry.time_since_epoch()).count();
        }
        fout << key << '\t' << quoted(record.value) << '\t' << expiryMilliSeconds << '\n';
    }
    return true;
}

bool DataBase::load(const string &filename)
{
    lock_guard<mutex> lock(mtx_);
    ifstream fin(filename);
    if (!fin)
        return false;
    kv_.clear();
    ordered_.clear();
    string line;
    while (getline(fin, line))
    {
        if (line.empty())
            continue;
        string key, value;
        long long expiryMilliSeconds;
        stringstream ss(line);
        // being defensive in here
        if (!(ss >> key))
            continue;
        if (!(ss >> quoted(value)))
            continue;
        if (!(ss >> expiryMilliSeconds))
            continue;
        Record record;
        record.value = value;
        if (expiryMilliSeconds != -1)
        {
            record.hasExpiry = true;
            record.expiry = system_clock::time_point(milliseconds(expiryMilliSeconds));
            if (system_clock::now() >= record.expiry)
                continue;   
        }
        kv_[key] = record;
        ordered_[key] = value;
    }
    return true;
}

bool DataBase::applyOperationsToBatch(const vector<Operation> &operations)
{
    lock_guard<mutex> lock(mtx_);
    for (const auto &op : operations)
    {
        if (op.type == Operation::Type::Set)
        {
            setLocked(op.key, op.value, op.timeToLiveSeconds);
        }
        else
        {
            delLocked(op.key);
        }
    }
    return true;
}
