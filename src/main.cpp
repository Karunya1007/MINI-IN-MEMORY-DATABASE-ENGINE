#include "database.h"
#include "wal.h"
#include "transaction.h"
#include "parser.h"
#include <iostream>
#include <thread>
#include <memory>
#include <chrono>
#include <atomic>

using namespace std;

static void printPairs(const vector<pair<string, string>> &items)
{
    if (items.empty())
    {
        cout << "(empty)\n";
        return;
    }
    for (auto &[key, value] : items)
    {
        cout << key << " => " << value << '\n';
    }
}

int main()
{
    DataBase db;
    WalLogger wal("wal.log");
    db.load("snapshot.db"); // load the recent saved snapshot
    wal.replay(db); // load the operations which are done after the last saved snapshot

    atomic<bool> running = true;
    thread cleanerthread([&]()
                   {
        while(running)
        {
            this_thread::sleep_for(chrono::seconds(5));
            db.cleanUpExpired();
        } });

    unique_ptr<Transaction> tx;
    cout << "Mini DB active: \n";
    string line;
    while (true)
    {
        cout << "db> ";
        if (!getline(cin, line))
            break;
        Command cmd = Parser::parse(line);
        switch (cmd.type)
        {
        case Command::Type::Set:
        {
            const string &key = cmd.args[0];
            const string &value = cmd.args[1];
            if (tx && tx->active())
            {
                // here we call the set function of transaction.
                tx->set(key, value, cmd.timeToLiveSeconds);
            }
            else
            {
                // if the operation is not a transaction then both wal and database gets updated
                wal.appendSet(key, value, cmd.timeToLiveSeconds);
                db.set(key, value, cmd.timeToLiveSeconds);
            }
            break;
        }
        case Command::Type::Get:
        {
            string valueOut;
            // directly searches in the database note that the new values added in the present transaction
            // or the old values getting updated in the present transaction wont be available.
            if (db.get(cmd.args[0], valueOut))
                cout << valueOut << '\n';
            else
                cout << "NULL \n";
            break;
        }
        case Command::Type::Del:
        {
            if (tx && tx->active())
            {
                tx->del(cmd.args[0]);
            }
            else
            {
                wal.appendDel(cmd.args[0]);
                cout << (db.del(cmd.args[0]) ? "Found and Deleted Successfully" : "Not Found") << '\n';
            }
            break;
        }
        case Command::Type::Exists:
        {
            cout << ((db.exists(cmd.args[0])) ? "Exists\n" : "Not Exists\n");
            break;
        }
        case Command::Type::Scan:
        {
            printPairs(db.scan());
            break;
        }
        case Command::Type::Range:
        {
            printPairs(db.rangeScan(cmd.args[0], cmd.args[1]));
            break;
        }
        case Command::Type::Save:
        {
            if (db.save(cmd.args[0]))
            {
                wal.clear();
                cout << "Saved\n";
            }
            else
            {
                cout << "Save Operation Failed\n";
            }
            break;
        }
        case Command::Type::Load:
        {
            if (db.load(cmd.args[0]))
            {
                cout << "Loaded\n";
            }
            else
            {
                cout << "Load Operation Failed\n";
            }
            break;
        }
        case Command::Type::Begin:
        {
            if (tx && tx->active())
            {
                cout << "Transaction already active. Commit the previous one to start a new transaction\n";
            }
            else
            {
                tx = make_unique<Transaction>(db, &wal);
                cout << "New Transaction Started\n";
            }
            break;
        }
        case Command::Type::Commit:
        {
            if ((!tx) || !(tx->active()))
            {
                cout << "No active Transaction\n";
            }
            else
            {
                cout << ((tx->commit()) ? "Committed\n" : "Commit failed\n");
                tx.reset();
            }
            break;
        }
        case Command::Type::Rollback:
        {
            if ((!tx) || !(tx->active()))
            {
                cout << "No active Transaction\n";
            }
            else
            {
                tx->rollBack();
                tx.reset();
                cout << "Rolled Back\n";
            }
            break;
        }
        case Command::Type::Exit:
        {
            db.save("snapshot.db");
            wal.clear();
            running = false;
            cleanerthread.join();
            cout << "Bye. Hope I served you well\n";
            return 0;
        }
        case Command::Type::Unknown:
        default:
            cout << "Unknown Command\n";
            break;
        }
    }
    running = false;
    if (cleanerthread.joinable()) cleanerthread.join();
    db.save("snapshot.db");
    wal.clear();
    return 0;
}
