#include "db/Transaction.h"

#include <exception>
#include <iostream>

namespace campus::db
{

Transaction::Transaction(MySqlConnection& connection)
    : connection_(connection)
{
    connection_.execute("START TRANSACTION");
}

Transaction::~Transaction()
{
    if (!finished_)
    {
        try
        {
            rollback();
        }
        catch (const std::exception& error)
        {
            std::cerr << "[WARN] transaction rollback failed during destruction: "
                      << error.what() << '\n';
        }
    }
}

void Transaction::commit()
{
    if (!finished_)
    {
        connection_.execute("COMMIT");
        finished_ = true;
    }
}

void Transaction::rollback()
{
    if (!finished_)
    {
        connection_.execute("ROLLBACK");
        finished_ = true;
    }
}

} // namespace campus::db
