#include "transaction.h"
using namespace std;

Transaction::Transaction(DataBase &db, WalLogger *wallog) : db_(db), wal_(wallog) {}

void Transaction::set(const string &key, const string &value, long long timeToLiveSeconds)
{
    if (!open_)
        return;
    // since transaction is active the operation is pushed into the operations vector
    operations_.emplace_back(DataBase::Operation::Type::Set, key, value, timeToLiveSeconds);
}

void Transaction::del(const string &key)
{
    if (!open_)
        return;
    operations_.emplace_back(DataBase::Operation::Type::Delete, key);
}

bool Transaction::commit()
{
    if (!open_)
        return false;
    if (wal_)
    {
        for (const auto &ops : operations_)
        {
            if (ops.type == DataBase::Operation::Type::Set)
            {
                if (!(wal_->appendSet(ops.key, ops.value, ops.timeToLiveSeconds)))
                    return false;
            }
            else
            {
                if (!(wal_->appendDel(ops.key)))
                    return false;
            }
        }
    }
    db_.applyOperationsToBatch(operations_);
    open_ = false;
    return true;
}

void Transaction::rollBack()
{
    operations_.clear();
    open_ = false;
    return;
}

bool Transaction::active() const
{
    return open_;
}