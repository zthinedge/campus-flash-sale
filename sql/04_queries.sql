-- Read-only queries for course demonstration and DAO implementation reference.

USE campus_flash_sale;
SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 1. Product list with seller information.
SELECT
    p.id,
    p.product_no,
    p.title,
    p.original_price_cents,
    p.status,
    u.username AS seller_username,
    p.created_at
FROM products AS p
JOIN users AS u ON u.id = p.seller_id
WHERE p.status <> 'OFF_SHELF'
ORDER BY p.created_at DESC;

-- 2. Activities that are available at the current database time.
SELECT
    a.id,
    a.activity_no,
    p.title,
    a.flash_price_cents,
    a.total_stock,
    a.available_stock,
    a.start_time,
    a.end_time
FROM flash_sale_activities AS a
JOIN products AS p ON p.id = a.product_id
WHERE a.status = 'ACTIVE'
  AND a.start_time <= NOW(3)
  AND a.end_time > NOW(3)
ORDER BY a.start_time, a.id;

-- 3. Activity detail and sold quantity.
SET @activity_no = 'ACT-BOOK-001';

SELECT
    a.id,
    a.activity_no,
    p.product_no,
    p.title,
    p.description,
    p.original_price_cents,
    a.flash_price_cents,
    a.total_stock,
    a.available_stock,
    a.total_stock - a.available_stock AS sold_quantity,
    a.status,
    a.start_time,
    a.end_time
FROM flash_sale_activities AS a
JOIN products AS p ON p.id = a.product_id
WHERE a.activity_no = @activity_no;

-- 4. Order list for one user.
SET @username = 'student03';

SELECT
    o.order_no,
    o.product_title_snapshot,
    o.purchase_price_cents,
    o.quantity,
    o.total_amount_cents,
    o.status,
    a.activity_no,
    o.created_at
FROM orders AS o
JOIN users AS u ON u.id = o.user_id
JOIN flash_sale_activities AS a ON a.id = o.activity_id
WHERE u.username = @username
ORDER BY o.created_at DESC;

-- 5. Administrator activity result summary.
SELECT
    a.activity_no,
    p.title,
    a.total_stock,
    a.available_stock,
    COUNT(o.id) AS created_order_count,
    COALESCE(SUM(o.total_amount_cents), 0) AS order_amount_cents
FROM flash_sale_activities AS a
JOIN products AS p ON p.id = a.product_id
LEFT JOIN orders AS o
    ON o.activity_id = a.id
   AND o.status = 'CREATED'
GROUP BY
    a.id,
    a.activity_no,
    p.title,
    a.total_stock,
    a.available_stock
ORDER BY a.id;

-- 6. Consistency check: expected stock based on inventory logs.
-- A correct result set is empty.
SELECT
    a.activity_no,
    a.total_stock,
    a.available_stock,
    a.total_stock + COALESCE(SUM(l.change_amount), 0) AS expected_stock
FROM flash_sale_activities AS a
LEFT JOIN inventory_logs AS l ON l.activity_id = a.id
GROUP BY a.id, a.activity_no, a.total_stock, a.available_stock
HAVING a.available_stock <> expected_stock;

-- 7. Duplicate-order check. The unique index should keep this result empty.
SELECT
    activity_id,
    user_id,
    COUNT(*) AS duplicate_count
FROM orders
GROUP BY activity_id, user_id
HAVING COUNT(*) > 1;

-- 8. Index demonstration for a user's latest orders.
EXPLAIN
SELECT
    order_no,
    product_title_snapshot,
    total_amount_cents,
    status,
    created_at
FROM orders
WHERE user_id = (SELECT id FROM users WHERE username = 'student03')
ORDER BY created_at DESC;
