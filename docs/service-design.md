# Service 层设计

## 1. Service 的职责

Service 面向业务用例，不直接处理 MySQL C API，也不负责 HTTP 解析。

```text
HTTP Handler / CLI
        |
        v
Service：参数、权限、状态、事务
        |
        v
DAO：SQL 和 Model 映射
        |
        v
db：连接、预处理语句、事务
```

DAO 回答“数据库执行结果是什么”，Service 将结果解释成“登录失败”“无权限”“售罄”或“重复下单”等业务结果。

## 2. 业务错误

`ServiceError` 为上层提供稳定的错误码：

```text
InvalidArgument
UsernameTaken
InvalidCredentials
UserNotFound
UserDisabled
Forbidden
ProductNotFound
ActivityNotFound
ActivityUnavailable
SoldOut
DuplicateOrder
```

未来 HTTP 层可以将它们映射为 `400`、`401`、`403`、`404` 或 `409`，而不需要解析 MySQL 错误字符串。

数据库唯一键错误 `1062` 会在相关 Service 中转换成用户名占用或重复下单。

## 3. AuthService

注册：

```text
校验用户名和密码
→ 查询用户名是否存在
→ SHA-256 计算密码摘要
→ UserDao 创建普通用户
```

登录：

```text
按用户名查询
→ 比较密码摘要
→ 检查账号状态
→ 返回用户
```

注册只有一次用户插入，不需要跨表事务。数据库用户名唯一索引仍是并发注册的最终防线。

## 4. ProductService

- 检查发布者存在且处于启用状态。
- 创建商品时由 Service 写入当前用户 ID，调用方不能伪造卖家。
- 提供商品详情和可见商品列表。

## 5. ActivityService

只有 `ADMIN` 可以创建抢购活动。

创建活动同时修改商品状态：

```text
START TRANSACTION
  INSERT flash_sale_activities
  UPDATE products SET status = 'IN_FLASH_SALE'
COMMIT
```

两步使用同一个连接。如果商品状态更新失败，活动插入也会回滚。

## 6. FlashSaleService

抢购是核心跨表事务：

```text
START TRANSACTION
  检查用户状态
  检查是否已经下单
  查询活动和商品快照
  条件扣减一件库存
  INSERT orders
  INSERT inventory_logs
COMMIT
```

一致性保证：

1. `available_stock > 0` 条件更新防止超卖。
2. `(activity_id, user_id)` 唯一索引防止重复下单。
3. 库存、订单和流水在同一连接、同一事务中。
4. 成功路径显式 `commit()`。
5. 异常、售罄、重复下单或提前返回时，RAII 析构自动回滚。

抢购价格和商品标题从数据库读取，不相信调用方提交的数据。

## 7. OrderService 与 AdminService

`OrderService` 只查询当前用户自己的订单。

`AdminService` 在验证管理员角色后，返回每场活动的：

- 当前库存。
- 订单数量。
- 库存流水数量。

普通用户调用管理员查询会得到 `Forbidden`。

## 8. 当前测试

`service_integration_test` 覆盖：

- 注册和登录。
- 密码错误与重复用户名。
- 发布商品。
- 普通用户创建活动被拒绝。
- 管理员创建活动。
- 抢购成功并扣减库存。
- 同一用户重复下单失败。
- 其他用户在库存为零时抢购失败。
- 个人订单查询。
- 管理员活动汇总与权限检查。

测试结束后按外键顺序删除临时数据，不污染课程演示数据。
