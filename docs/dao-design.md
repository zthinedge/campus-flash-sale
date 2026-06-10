# DAO 层设计

## 1. 依赖方向

```text
Service
   |
   v
DAO
   |
   v
PreparedStatement / MySqlConnection / Transaction
   |
   v
MySQL
```

- `db` 层只解决 MySQL 连接、参数绑定、结果读取和事务生命周期。
- `dao` 层封装业务表的 SQL，并把数据库记录映射成 `model`。
- `service` 层负责权限、业务规则以及跨多个 DAO 的事务编排。

DAO 不依赖 HTTP，也不决定某个用户是否具有管理员权限。

## 2. PreparedStatement

DAO 不拼接外部输入，而是使用参数占位符：

```sql
SELECT id, username
FROM users
WHERE username = ?
```

`PreparedStatement` 负责：

- 检查参数数量。
- 绑定字符串、有符号整数、无符号整数和 `NULL`。
- 执行更新并返回 `insertId`、`affectedRows`。
- 执行查询并按列名读取结果。
- 析构时关闭 `MYSQL_STMT`。

SQL 固定结构可以在 C++ 中拼接，例如拼接固定的列名常量；用户输入必须通过参数绑定传递。

## 3. DAO 职责

### UserDao

- 按 ID 或用户名查询用户。
- 创建用户。

注册是否允许、密码是否正确属于 `AuthService`，不属于 DAO。

### ProductDao

- 查询商品。
- 查询可见商品列表。
- 创建商品。

卖家是否有发布权限由 `ProductService` 判断。

### ActivityDao

- 查询和创建抢购活动。
- 条件扣减一件库存。

`deductOne()` 使用单条条件更新：

```sql
UPDATE flash_sale_activities
SET available_stock = available_stock - 1
WHERE id = ?
  AND status = 'ACTIVE'
  AND start_time <= NOW(3)
  AND end_time > NOW(3)
  AND available_stock > 0;
```

只有 `affectedRows == 1` 才表示扣减成功。这比“先查询库存，再更新库存”更安全，因为检查和扣减在数据库中原子完成。

### OrderDao

- 创建订单。
- 按订单号或用户查询订单。
- 统计活动订单数。

同一用户不能重复抢购由 `(activity_id, user_id)` 唯一索引最终保证。

### InventoryLogDao

- 写入库存流水。
- 查询和统计活动库存流水。

库存流水是业务审计数据，不是程序运行日志。

## 4. 连接与事务

DAO 不创建和关闭连接，只保存外部传入的连接引用：

```cpp
ActivityDao activityDao(connection);
OrderDao orderDao(connection);
InventoryLogDao inventoryLogDao(connection);
```

同一个业务事务中的 DAO 必须使用同一个 `MySqlConnection`：

```cpp
Transaction transaction(connection);

activityDao.deductOne(activityId);
orderDao.create(order);
inventoryLogDao.create(log);

transaction.commit();
```

若中途抛出异常或提前返回，`Transaction` 析构时回滚该连接上的整个事务。

## 5. Service 调用方式

当前 `FlashSaleService` 负责：

1. 查询用户和活动。
2. 开启事务。
3. 调用 `ActivityDao::deductOne()`。
4. 创建订单。
5. 创建库存流水。
6. 显式提交事务。

DAO 只报告数据库结果，Service 决定将结果解释为“抢购成功”“库存不足”或“重复下单”。
