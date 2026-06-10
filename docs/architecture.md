# 系统架构

## 1. 架构目标

campus-flash-sale 第一版采用单进程、分层式 C++ 后端架构，完成校园闲置商品限时抢购的核心闭环。当前重点是隔离浏览器交互、HTTP 网络、业务逻辑和数据库访问，并依靠 MySQL 事务与约束保证数据一致性。

第一版不引入 Redis、消息队列、分布式锁或微服务。

## 2. 当前验收架构

```text
Browser
    |
    v
HTML / CSS / JavaScript
    |
    v
epoll Reactor + HTTP/1.1
    |
    v
WebApp JSON API
    |
    v
Service Layer
    |
    v
DAO Layer
    |
    v
db Layer
  - MySqlConnection
  - PreparedStatement
  - Transaction
    |
    v
MySQL 5.7 / InnoDB
```

浏览器只通过 JSON API 访问后端，不直接接触数据库。`WebApp` 负责 HTTP 与 JSON 转换、演示级内存会话和错误码映射，不编写库存 SQL。CLI 仍通过 `--cli` 保留为备用入口。

当前 Web 服务使用一个数据库连接并在 epoll 事件循环中同步执行 Service。该方式适合课程验收，但数据库访问会阻塞事件循环；后续应增加业务线程池和连接池。

## 3. 后续高并发架构

```text
HTTP Client
    |
io_uring Network Layer
    |
Bounded Task Queue
    |
Business Thread Pool
    |
Service -> DAO -> MySQL Connection Pool -> MySQL
```

MySQL C API 属于阻塞式接口。后续接入 io_uring 时，数据库操作应放在业务线程中，不能阻塞网络事件循环。

## 4. 模块职责

### net

当前封装 epoll、监听套接字、连接生命周期、缓冲区、HTTP 请求解析、路由和静态资源服务。后续可增加 io_uring 实现。

### thread

预留给固定大小业务线程池和有界任务队列。当前 Web 服务尚未将数据库任务移出网络线程，并发测试使用 `std::thread` 验证数据库一致性。

### db

封装 MySQL 连接、预处理语句和 RAII 事务。事务期间的所有 DAO 操作必须使用同一个数据库连接；连接池尚未实现。

### dao

负责用户、商品、抢购活动和订单的数据访问，只表达持久化操作，不编排跨表业务流程。

### service

负责参数校验、权限判断、业务状态检查和事务编排。抢购流程的库存扣减、订单创建及库存流水写入在此层组成一个事务。

### model

保存数据库实体、请求与响应 DTO、状态枚举。模型层不持有网络连接或数据库连接。

### utils

当前提供 JSON 配置读取和密码摘要工具；日志、时间等通用能力可继续扩展。

## 5. 并发与事务边界

核心抢购流程由 MySQL 事务保证原子性：

1. 使用 `available_stock > 0` 的条件更新校验活动并原子扣减库存。
2. 创建订单，并由 `(activity_id, user_id)` 唯一索引阻止重复下单。
3. 写入库存流水。
4. 任一步骤失败则回滚整个事务。

C++ 层的库存检查仅用于提前返回，数据库条件更新和唯一约束才是并发一致性的最终防线。

## 6. 当前部署模型

当前版本采用一个 C++ Web 服务进程、一个 epoll 事件循环和一个 MySQL 实例。静态前端由同一服务进程提供，配置从本地 `config/config.json` 读取，该文件包含数据库凭据且不得提交到版本库。

后续再引入 io_uring、业务线程池和连接池。Redis、消息队列、多实例部署不属于课程设计交付范围。
