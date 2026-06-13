#pragma once
// used to ensure that header is included only once during a single compilation
#include<string>
#include<vector>
#include<map>
#include<unordered_map>
#include<chrono>
#include<mutex>

class DataBase
{
public:
    struct Operation
    {
        enum class Type {Set, Delete};
        Type type;
        std::string key;
        std::string value;
        long long timeToLiveSeconds;

        Operation(Type t, std::string k, std::string v="", long long ttl=-1): type(std::move(t)), key(std::move(k)), value(std::move(v)), timeToLiveSeconds(std::move(ttl)){}
    };
    
    bool set(const std::string& key, const std::string& val, long long timeToLiveSeconds = -1);
    bool get(const std::string& key, std::string& valueOut);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    std::vector<std::pair<std::string, std::string>> scan();
    std::vector<std::pair<std::string, std::string>> rangeScan(const std::string& start, const std::string& end);
    void cleanUpExpired();
    bool save(const std::string& filename);
    bool load(const std::string& filename);
    bool applyOperationsToBatch(const std::vector<Operation>& ops);

private:
    struct Record
    {
        std::string value;
        bool hasExpiry = false;
        std::chrono::system_clock::time_point expiry;
    };

    std::unordered_map<std::string, Record> kv_; // faster lookup
    std::map<std::string, std::string> ordered_; // sorted traversal
    std::mutex mtx_;

    bool isExpiredLocked(const Record& record)const;
    void eraseKeyLocked(const std::string& key);
    bool setLocked(const std::string& key, const std::string& value, long long timeToLiveSeconds);
    bool delLocked(const std::string& key);
};
