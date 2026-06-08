#pragma once

#include "db/MySqlConnection.h"

namespace campus::db
{

class Transaction
{
public:
    explicit Transaction(MySqlConnection& connection);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(Transaction&&) = delete;

    void commit();
    void rollback();

private:
    MySqlConnection& connection_;
    bool finished_ { false };
};

} // namespace campus::db
