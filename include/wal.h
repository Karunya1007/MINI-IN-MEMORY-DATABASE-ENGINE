#pragma once
#include <string>
class DataBase;

class WalLogger
{
public:
    explicit WalLogger(const std::string& filename);
    bool appendSet(const std::string& key, const std::string& value, long long timeToLiveSeconds = -1);
    bool appendDel(const std::string& key);
    bool clear();
    bool replay(DataBase& db);

private:
    std::string filename_;
};
