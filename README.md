# campus-flash-sale

基于 C++20 与 MySQL 事务一致性的校园限时交易系统。

本项目面向数据库课程设计与 C++ 后端工程实践。当前验收版本提供浏览器展示界面，通过 C++ HTTP 服务完成用户、商品、活动、抢购和订单闭环，重点展示数据库设计、Service/DAO 分层、RAII 事务、防超卖与防重复下单。

当前网络层复用 ReactorHttpKit 的 epoll Reactor 与 HTTP/1.1 实现。io_uring、业务线程池和 MySQL 连接池属于后续工程化扩展。

## 技术栈

- C++20
- CMake
- Linux
- epoll Reactor、Socket 与 HTTP/1.1
- MySQL 5.7、InnoDB 与 MySQL C API
- OpenSSL SHA-256（课程演示密码摘要）
- nlohmann/json（配置读取）
- HTML5 / CSS3 / 原生 JavaScript
- pthread / `std::thread`

## 核心亮点

- MySQL 连接、预处理语句和 RAII 事务封装
- Service / DAO 分层
- C++ HTTP JSON API 与响应式浏览器界面
- 条件库存扣减防止超卖
- 数据库唯一索引防止重复下单
- 库存、订单和库存流水在同一事务中提交或回滚
- 30 用户并发抢购测试与最终数据一致性验证

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
src/           C++ 实现、HTTP 服务、命令行应用及程序入口
sql/           建表、索引、测试数据和核心查询
docs/          架构、数据库与事务设计文档
tests/         单元、集成与并发测试
config/        配置示例；真实配置不提交
web/           浏览器前端页面、样式与交互脚本
```

DAO 分层、预处理语句和事务连接复用说明见 `docs/dao-design.md`。
业务用例、权限和事务边界说明见 `docs/service-design.md`。
现场演示步骤见 `docs/acceptance-guide.md`，测试结果见 `docs/test-report.md`。
Web 页面和 API 说明见 `docs/web-interface.md`。

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

Ubuntu 环境安装构建依赖：

```bash
sudo apt install build-essential cmake libmysqlclient-dev libssl-dev nlohmann-json3-dev
```

将 `config/config.json` 中的 `mysql_password` 改为本机 `campus_app`
密码。该文件已被 Git 忽略，不会提交到仓库。

当前程序会读取配置、验证 MySQL 连接和事务封装，然后启动 Web 服务：

```bash
cmake -S . -B build
cmake --build build
./build/campus_flash_sale
```

浏览器访问：

```text
http://127.0.0.1:8080
```

如果服务运行在虚拟机、浏览器位于宿主机，先在虚拟机执行 `hostname -I`
获取 IP，然后访问 `http://虚拟机IP:8080`。默认配置监听 `0.0.0.0`。

演示账号：

- 管理员：`admin / admin123`
- 普通用户：`student01 / 123456`

也可以显式指定配置文件、进入备用 CLI，或仅校验配置：

```bash
./build/campus_flash_sale config/config.json
./build/campus_flash_sale --cli config/config.json
./build/campus_flash_sale --check-config config/config.example.json
```

运行测试：

```bash
ctest --test-dir build --output-on-failure
```

默认构建包含不依赖数据库的配置测试。完整 MySQL 集成测试建议使用独立构建目录：

```bash
cmake -S . -B build-mysql-test \
  -DCAMPUS_TEST_CONFIG="$PWD/config/config.json"
cmake --build build-mysql-test
ctest --test-dir build-mysql-test --output-on-failure
./build-mysql-test/tests/flash_sale_concurrency_test config/config.json
```

## 后续规划

1. 将 epoll 网络层升级或扩展为 io_uring 实现。
2. 实现有界业务线程池，避免数据库操作阻塞事件循环。
3. 将单连接模式升级为 MySQL 连接池。
4. 将原生前端升级为 Vue 工程并完善路由和组件测试。
5. 使用 `wrk` 等工具记录吞吐、延迟分位数和资源使用情况。
