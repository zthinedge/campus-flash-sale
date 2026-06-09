#pragma once

#include "db/MySqlConnection.h"
#include "model/FlashSaleActivity.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace campus::dao
{

class ActivityDao
{
public:
    explicit ActivityDao(db::MySqlConnection& connection);

    std::optional<model::FlashSaleActivity> findById(std::uint64_t id);
    std::optional<model::FlashSaleActivity> findByActivityNo(std::string_view activityNo);
    std::vector<model::FlashSaleActivity> listActive();
    std::uint64_t create(const model::CreateFlashSaleActivity& input);
    bool deductOne(std::uint64_t activityId);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::dao
