# campus-flash-sale

基于 C++20、io_uring 与 MySQL 事务一致性的高并发校园限时交易系统。

本项目面向数据库课程设计与 C++ 后端工程实践。第一版聚焦校园闲置商品限时抢购的核心闭环，通过清晰分层逐步实现网络 I/O、业务线程池、MySQL 连接池以及库存与订单的一致性控制。

当前仓库处于工程骨架阶段，仅包含构建系统、目录分层、配置示例和启动程序，尚未接入数据库与 io_uring。

## 技术栈

- C++20
- CMake
- Linux
- liburing（后续接入）
- MySQL 5.7 与 MySQL C API（后续接入）
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

DAO 分层、预处理语句和事务连接复用说明见 `docs/dao-design.md`。
业务用例、权限和事务边界说明见 `docs/service-design.md`。

## 编译运行

首次运行前，使用 MySQL 管理员账号初始化数据库。执行前必须修改
`sql/01_create_database.sql` 中的示例密码：

```bash
mysql -u root -p < sql/01_create_database.sql
mysql -u root -p < sql/02_schema.sql
mysql -u root -p < sql/03_seed.sql
cp config/config.example.json config/config.json
```

数据库设计和账号说明见 `docs/database-design.md`。

安装 JSON 配置解析依赖：

```bash
sudo apt install nlohmann-json3-dev
```

将 `config/config.json` 中的 `mysql_password` 改为本机 `campus_app`
密码。该文件已被 Git 忽略，不会提交到仓库。

当前 C++ 程序会读取配置、连接 MySQL，并通过 `SELECT 1` 验证连接：

```bash
cmake -S . -B build
cmake --build build
./build/campus_flash_sale
```

启动成功后会额外开启并提交一个只读事务，用于验证 RAII 事务封装。

也可以显式指定配置文件，或仅校验配置而不连接数据库：

```bash
./build/campus_flash_sale config/config.example.json
./build/campus_flash_sale --check-config config/config.example.json
```

运行测试：

```bash
ctest --test-dir build --output-on-failure
```

默认测试不依赖本地数据库。若要运行 MySQL 事务集成测试：

```bash
cmake -S . -B build -DCAMPUS_TEST_CONFIG="$PWD/config/config.json"
cmake --build build
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
