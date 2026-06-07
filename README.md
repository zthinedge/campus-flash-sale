# campus-flash-sale

基于 C++20、io_uring 与 MySQL 事务一致性的高并发校园限时交易系统。

本项目面向数据库课程设计与 C++ 后端工程实践。第一版聚焦校园闲置商品限时抢购的核心闭环，通过清晰分层逐步实现网络 I/O、业务线程池、MySQL 连接池以及库存与订单的一致性控制。

当前仓库处于工程骨架阶段，仅包含构建系统、目录分层、配置示例和启动程序，尚未接入数据库与 io_uring。

## 技术栈

- C++20
- CMake
- Linux
- liburing（后续接入）
- MySQL 8.x 与 MySQL C API（后续接入）
- pthread / `std::thread`
- HTTP/1.1 与 JSON（后续实现）

## 核心亮点

- 基于 io_uring 的异步网络 I/O
- 网络线程与业务线程池解耦
- MySQL 连接池与 RAII 事务封装
- Service / DAO 分层
- 条件库存扣减防止超卖
- 数据库唯一索引防止重复下单
- 并发抢购测试与压测结果验证

## 第一版功能范围

- 用户注册与简化登录
- 商品发布、列表查询和详情查询
- 创建和查询限时抢购活动
- 用户参与抢购并生成订单
- 用户订单列表查询
- 管理员或测试接口查询活动、库存和订单结果
- 使用 MySQL 事务与唯一索引保证库存和订单一致性

第一版不包含 Redis、消息队列、分布式部署、真实支付、购物车、物流、复杂鉴权和完整商业前端。

## 目录结构

```text
include/       公共头文件，按 net、thread、db、dao、service、model、utils 分层
src/           C++ 实现及程序入口
sql/           建表、索引、测试数据和核心查询
docs/          架构、数据库与事务设计文档
tests/         单元、集成与并发测试
config/        配置示例；真实配置不提交
```

## 编译运行

```bash
cmake -S . -B build
cmake --build build
./build/campus_flash_sale
```

运行测试：

```bash
ctest --test-dir build --output-on-failure
```

## 后续规划

1. 完成 HTTP 协议解析和基础路由。
2. 接入 io_uring，实现连接与异步读写管理。
3. 实现有界业务线程池和任务回传机制。
4. 接入 MySQL，完成连接池、预处理语句和事务封装。
5. 实现用户、商品、活动、抢购与订单模块。
6. 补充 ER 图、关系模式、建表 SQL、测试数据及核心查询。
7. 编写并发一致性测试并记录 QPS、延迟和失败原因。
