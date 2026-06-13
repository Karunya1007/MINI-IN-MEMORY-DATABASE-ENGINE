#pragma once
#include <string>
#include <vector>

class Command
{
public:
    enum class Type 
    {
        Set, Get, Del, Exists, Scan, Range, Save, Load, Begin, Commit, Rollback, Exit, Unknown
    };
    Type type = Type::Unknown;
    std::vector<std::string> args;
    long long timeToLiveSeconds = -1;
};

class Parser
{
public:
    static Command parse(const std::string& line);
};