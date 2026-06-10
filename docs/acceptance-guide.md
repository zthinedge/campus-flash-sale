# 验收指南

## 1. 交付范围

当前版本是可运行的数据库课程设计系统，主要交互入口为浏览器：

- 普通用户注册、登录、发布商品、查看活动、参与抢购、查看订单。
- 管理员创建活动并查看活动库存、订单和库存流水汇总。
- 响应式 Web 页面与 C++ JSON API，页面数据均来自 MySQL。
- MySQL 表、索引、外键、触发器、测试数据和核心查询。
- 抢购事务、防超卖、防重复下单和并发一致性测试。

当前 HTTP 网络层使用 epoll Reactor；Vue、io_uring、业务线程池和连接池是后续扩展。

## 2. 初始化

先修改 `sql/01_create_database.sql` 中的示例密码，然后执行：

```bash
mysql -u root -p < sql/01_create_database.sql
mysql -u root -p < sql/02_schema.sql
mysql -u root -p < sql/03_seed.sql
cp config/config.example.json config/config.json
```

将 `config/config.json` 的 `mysql_password` 改为 `campus_app` 的实际密码。

## 3. 编译运行

```bash
cmake -S . -B build
cmake --build build
./build/campus_flash_sale config/config.json
```

同一台机器的浏览器打开 `http://127.0.0.1:8080`。如果 C++ 服务运行在虚拟机、
浏览器位于宿主机，执行 `hostname -I` 获取虚拟机 IP，然后打开
`http://虚拟机IP:8080`。备用命令行入口：

```bash
./build/campus_flash_sale --cli config/config.json
```

演示账号：

| 身份 | 用户名 | 密码 |
| --- | --- | --- |
| 管理员 | `admin` | `admin123` |
| 普通用户 | `student01` | `123456` |
| 普通用户 | `student02` | `123456` |

## 4. 建议演示顺序

1. 展示 `docs/database-design.md` 的 ER 图、关系模式和索引设计。
2. 打开网页首页，展示浏览器 → HTTP → Service → DAO → MySQL 架构链路。
3. 进入“限时抢购”，以游客身份查看商品和进行中的活动。
4. 使用 `admin` 登录，进入“管理看板”，说明库存、订单、流水三者关系。
5. 退出后使用 `student01` 登录，在抢购大厅购买 `ACT-BOOK-001`。
6. 查看“我的订单”，再次购买同一活动，页面应提示重复下单。
7. 使用管理员账号再次查看活动汇总，确认库存减少、订单和流水同时增加。
8. 执行并发测试，展示 30 人抢 10 件时仅有 10 笔订单。

若重复演示导致种子库存发生变化，可在没有测试进程运行时执行：

```bash
mysql -u root -p < sql/reset.sql
mysql -u root -p < sql/01_create_database.sql
mysql -u root -p < sql/02_schema.sql
mysql -u root -p < sql/03_seed.sql
```

## 5. 权限说明

| 功能 | 游客 | 普通用户 | 管理员 |
| --- | --- | --- | --- |
| 查看商品和活动 | 是 | 是 | 是 |
| 注册、登录 | 是 | - | - |
| 发布商品 | 否 | 是 | 是 |
| 参与抢购、查看自己的订单 | 否 | 是 | 是 |
| 创建活动、查看全局汇总 | 否 | 否 | 是 |

业务角色保存在 `users.role` 中，与 MySQL 登录账号 `campus_app` 是两个不同概念。所有业务用户共享服务端数据库账号，由 Service 层进行业务权限判断。

## 6. 自动化测试

```bash
cmake -S . -B build-mysql-test \
  -DCAMPUS_TEST_CONFIG="$PWD/config/config.json"
cmake --build build-mysql-test
ctest --test-dir build-mysql-test --output-on-failure
./build-mysql-test/tests/flash_sale_concurrency_test config/config.json
```

最后一条命令会打印并发人数、初始库存、成功数、拒绝数、最终库存、订单数和库存流水数。
