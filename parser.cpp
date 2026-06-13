#include "parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
using namespace std;

bool notSpace(unsigned char c)
{
    return !isspace(c);
}

static string trim(string s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), notSpace));
    s.erase(find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

Command Parser::parse(const string& line) {
    Command cmd;
    string s = trim(line);
    if(s.empty())return cmd;
    stringstream ss(s);
    string op;
    ss >> op;
    for(auto& ch: op)
    {
        ch = (char)toupper((unsigned char)ch);
    }
    if(op == "SET")
    {
        string key;
        ss >> key;
        string rest;
        getline(ss, rest);
        rest = trim(rest);
        if(key.empty() || rest.empty())return cmd;
        cmd.type = Command::Type::Set;
        cmd.args.push_back(key);
        string value;
        long long ttlSeconds = -1;
        stringstream rs(rest);
        if(rest[0] == '"')
        {
            rs >> quoted(value);
            string token;
            if(rs >> token)
            {
                if(token == "TTL")
                {
                    rs >> ttlSeconds;
                }
            }
        }
        else
        {
            rs >> value;
            string token;
            if(rs >> token)
            {
                if(token == "TTL")
                {
                    rs >> ttlSeconds;
                }
            }
        }
        cmd.args.push_back(value);
        cmd.timeToLiveSeconds = ttlSeconds;
        return cmd;
    }
    if (op == "GET")
    {
        string key;
        if (ss >> key) {
            cmd.type = Command::Type::Get;
            cmd.args.push_back(key);
        }
        return cmd;
    }
    if (op == "DEL" || op == "DELETE")
    {
        string key;
        if (ss >> key) 
        {
            cmd.type = Command::Type::Del;
            cmd.args.push_back(key);
        }
        return cmd;
    }
    if (op == "EXISTS")
    {
        string key;
        if (ss >> key)
        {
            cmd.type = Command::Type::Exists;
            cmd.args.push_back(key);
        }
        return cmd;
    }
    if (op == "SCAN")
    {
        cmd.type = Command::Type::Scan;
        return cmd;
    }
    if (op == "RANGE")
    {
        string a, b;
        if (ss >> a >> b) {
            cmd.type = Command::Type::Range;
            cmd.args.push_back(a);
            cmd.args.push_back(b);
        }
        return cmd;
    }
    if (op == "SAVE")
    {
        string file;
        if (ss >> file)
        {
            cmd.type = Command::Type::Save;
            cmd.args.push_back(file);
        }
        return cmd;
    }
    if (op == "LOAD")
    {
        string file;
        if (ss >> file)
        {
            cmd.type = Command::Type::Load;
            cmd.args.push_back(file);
        }
        return cmd;
    }
    if (op == "BEGIN")
    {
        cmd.type = Command::Type::Begin;
        return cmd;
    }
    if (op == "COMMIT")
    {
        cmd.type = Command::Type::Commit;
        return cmd;
    }
    if (op == "ROLLBACK")
    {
        cmd.type = Command::Type::Rollback;
        return cmd;
    }
    if (op == "EXIT" || op == "QUIT")
    {
        cmd.type = Command::Type::Exit;
        return cmd;
    }
    return cmd;
}