# 系统架构

## 1. 架构目标

campus-flash-sale 第一版采用单进程、分层式 C++ 后端架构，完成校园闲置商品限时抢购的核心闭环。设计重点是隔离网络 I/O、业务计算和阻塞式数据库访问，并依靠 MySQL 事务与约束保证最终一致性。

第一版不引入 Redis、消息队列、分布式锁或微服务。

## 2. 总体架构

```text
HTTP Client
    |
    v
io_uring Network Layer
  - accept / recv / send
  - connection lifecycle
  - HTTP request parsing
    |
    v
Bounded Task Queue
    |
    v
Business Thread Pool
    |
    v
Service Layer
    |
    v
DAO Layer
    |
    v
MySQL Connection Pool -> MySQL 8.x
```

MySQL C API 属于阻塞式接口，数据库操作不得运行在 io_uring 网络线程中。网络线程负责连接和 I/O 事件，业务线程负责 Service、DAO 和数据库事务，处理结果再交回网络层发送。

## 3. 模块职责

### net

封装 io_uring 实例、监听套接字、连接生命周期、读写缓冲区和 HTTP 网络入口。该模块不包含具体业务规则。

### thread

提供固定大小的业务线程池和有界任务队列。队列达到上限时应快速拒绝请求，避免高并发下任务无限堆积。

### db

封装 MySQL 连接、连接池、预处理语句和 RAII 事务。事务期间的所有 DAO 操作必须使用同一个数据库连接。

### dao

负责用户、商品、抢购活动和订单的数据访问，只表达持久化操作，不编排跨表业务流程。

### service

负责参数校验、权限判断、业务状态检查和事务编排。抢购流程的库存扣减、订单创建及库存流水写入在此层组成一个事务。

### model

保存数据库实体、请求与响应 DTO、状态枚举。模型层不持有网络连接或数据库连接。

### utils

提供配置、日志、时间、字符串、订单号和通用错误处理等基础能力。

## 4. 并发与事务边界

核心抢购流程由 MySQL 事务保证原子性：

1. 校验活动状态和时间。
2. 创建订单，并由 `(activity_id, user_id)` 唯一索引阻止重复下单。
3. 使用 `available_stock > 0` 的条件更新原子扣减库存。
4. 写入库存流水。
5. 任一步骤失败则回滚整个事务。

C++ 层的库存检查仅用于提前返回，数据库条件更新和唯一约束才是并发一致性的最终防线。

## 5. 第一版部署模型

第一版采用一个服务进程、一个 io_uring 网络线程、固定大小业务线程池和一个 MySQL 实例。配置从本地 `config/config.json` 读取，该文件包含数据库凭据且不得提交到版本库。

后续可在压测结果基础上评估多网络线程、多实例部署、缓存和异步消息，但这些不属于第一版范围。
