-- Transaction template for one flash-sale request.
--
-- The C++ service must inspect affected_rows after the UPDATE:
--   1: continue the transaction
--   0: ROLLBACK and return activity-unavailable or sold-out
--
-- A duplicate (activity_id, user_id) INSERT raises a duplicate-key error.
-- The C++ RAII transaction must then ROLLBACK, restoring the deducted stock.

USE campus_flash_sale;
SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Demo input. The real service binds these values as prepared parameters.
SET @activity_id = (
    SELECT id
    FROM flash_sale_activities
    WHERE activity_no = 'ACT-BOOK-001'
);
SET @user_id = (
    SELECT id
    FROM users
    WHERE username = 'student04'
);
SET @order_no = CONCAT(
    'ORD-DEMO-',
    DATE_FORMAT(NOW(3), '%Y%m%d%H%i%s%f')
);

START TRANSACTION;

UPDATE flash_sale_activities
SET available_stock = available_stock - 1
WHERE id = @activity_id
  AND status = 'ACTIVE'
  AND start_time <= NOW(3)
  AND end_time > NOW(3)
  AND available_stock > 0;

-- The application checks mysql_affected_rows() here.
SELECT ROW_COUNT() AS deducted_rows;

INSERT INTO orders (
    order_no,
    user_id,
    activity_id,
    product_id,
    product_title_snapshot,
    purchase_price_cents,
    quantity,
    total_amount_cents,
    status
)
SELECT
    @order_no,
    @user_id,
    a.id,
    p.id,
    p.title,
    a.flash_price_cents,
    1,
    a.flash_price_cents,
    'CREATED'
FROM flash_sale_activities AS a
JOIN products AS p ON p.id = a.product_id
WHERE a.id = @activity_id;

SET @order_id = LAST_INSERT_ID();

INSERT INTO inventory_logs (
    activity_id,
    order_id,
    change_amount,
    stock_after,
    reason
)
SELECT
    a.id,
    @order_id,
    -1,
    a.available_stock,
    'FLASH_SALE_DEDUCT'
FROM flash_sale_activities AS a
WHERE a.id = @activity_id;

COMMIT;
