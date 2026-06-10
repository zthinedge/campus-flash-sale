# Web 展示层

## 1. 页面功能

浏览器访问 `http://127.0.0.1:8080`，包含四个视图：

- 系统总览：数据库状态、活动数量、库存数量和系统架构。
- 限时抢购：活动卡片、商品列表、库存进度和抢购操作。
- 我的订单：展示当前登录用户的订单。
- 管理看板：展示所有活动的库存、订单数、库存流水数及一致性状态。

页面还支持注册、登录、退出、发布商品和管理员创建活动。

## 2. API

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| `GET` | `/api/health` | 服务与数据库健康检查 |
| `POST` | `/api/register` | 注册普通用户并登录 |
| `POST` | `/api/login` | 登录 |
| `POST` | `/api/logout` | 退出 |
| `GET` | `/api/me` | 查询当前登录用户 |
| `GET` | `/api/products` | 商品列表 |
| `GET` | `/api/products/{id}` | 商品详情 |
| `POST` | `/api/products` | 发布商品 |
| `GET` | `/api/activities` | 进行中的活动 |
| `POST` | `/api/activities` | 管理员创建活动 |
| `POST` | `/api/activities/{id}/purchase` | 参与抢购 |
| `GET` | `/api/orders` | 当前用户订单 |
| `GET` | `/api/admin/summary` | 管理员一致性汇总 |

## 3. 分层原则

`WebApp` 只负责：

- HTTP 路由与 JSON 转换。
- 演示级登录会话。
- 将业务错误映射为 HTTP 状态码。
- 调用已有 Service。

库存条件更新、唯一索引和事务编排仍在 Service、DAO 和 MySQL 中完成，前端不能绕过一致性规则。
