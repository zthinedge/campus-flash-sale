-- Target: MySQL 5.7
-- MySQL 5.7 parses but ignores CHECK constraints, so critical cross-column
-- validation is implemented with BEFORE triggers at the end of this file.

USE campus_flash_sale;
SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role ENUM('USER', 'ADMIN') NOT NULL DEFAULT 'USER',
    status ENUM('ACTIVE', 'DISABLED') NOT NULL DEFAULT 'ACTIVE',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_users_username (username),
    KEY idx_users_role_status (role, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS products (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    product_no VARCHAR(32) NOT NULL,
    seller_id BIGINT UNSIGNED NOT NULL,
    title VARCHAR(128) NOT NULL,
    description VARCHAR(1000) NOT NULL DEFAULT '',
    original_price_cents INT UNSIGNED NOT NULL,
    status ENUM(
        'ON_SALE',
        'IN_FLASH_SALE',
        'SOLD',
        'OFF_SHELF'
    ) NOT NULL DEFAULT 'ON_SALE',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_products_product_no (product_no),
    KEY idx_products_seller_created (seller_id, created_at),
    KEY idx_products_status_created (status, created_at),
    CONSTRAINT fk_products_seller
        FOREIGN KEY (seller_id) REFERENCES users (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS flash_sale_activities (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    activity_no VARCHAR(32) NOT NULL,
    product_id BIGINT UNSIGNED NOT NULL,
    flash_price_cents INT UNSIGNED NOT NULL,
    total_stock INT UNSIGNED NOT NULL,
    available_stock INT UNSIGNED NOT NULL,
    start_time DATETIME(3) NOT NULL,
    end_time DATETIME(3) NOT NULL,
    status ENUM(
        'PENDING',
        'ACTIVE',
        'ENDED',
        'CANCELED'
    ) NOT NULL DEFAULT 'PENDING',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_flash_sale_activities_activity_no (activity_no),
    KEY idx_flash_sale_activities_product (product_id),
    KEY idx_flash_sale_activities_status_time (status, start_time, end_time),
    CONSTRAINT fk_flash_sale_activities_product
        FOREIGN KEY (product_id) REFERENCES products (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS orders (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    order_no VARCHAR(32) NOT NULL,
    user_id BIGINT UNSIGNED NOT NULL,
    activity_id BIGINT UNSIGNED NOT NULL,
    product_id BIGINT UNSIGNED NOT NULL,
    product_title_snapshot VARCHAR(128) NOT NULL,
    purchase_price_cents INT UNSIGNED NOT NULL,
    quantity TINYINT UNSIGNED NOT NULL DEFAULT 1,
    total_amount_cents INT UNSIGNED NOT NULL,
    status ENUM('CREATED', 'CANCELED') NOT NULL DEFAULT 'CREATED',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_orders_order_no (order_no),
    UNIQUE KEY uk_orders_activity_user (activity_id, user_id),
    KEY idx_orders_user_created (user_id, created_at),
    KEY idx_orders_activity_status (activity_id, status),
    KEY idx_orders_product (product_id),
    CONSTRAINT fk_orders_user
        FOREIGN KEY (user_id) REFERENCES users (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_orders_activity
        FOREIGN KEY (activity_id) REFERENCES flash_sale_activities (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_orders_product
        FOREIGN KEY (product_id) REFERENCES products (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS inventory_logs (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    activity_id BIGINT UNSIGNED NOT NULL,
    order_id BIGINT UNSIGNED NULL,
    change_amount SMALLINT NOT NULL,
    stock_after INT UNSIGNED NOT NULL,
    reason ENUM(
        'FLASH_SALE_DEDUCT',
        'ORDER_CANCEL_RESTORE',
        'ADMIN_ADJUST'
    ) NOT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (id),
    UNIQUE KEY uk_inventory_logs_order_reason (order_id, reason),
    KEY idx_inventory_logs_activity_created (activity_id, created_at),
    CONSTRAINT fk_inventory_logs_activity
        FOREIGN KEY (activity_id) REFERENCES flash_sale_activities (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_inventory_logs_order
        FOREIGN KEY (order_id) REFERENCES orders (id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELIMITER $$

DROP TRIGGER IF EXISTS trg_users_before_insert$$
CREATE TRIGGER trg_users_before_insert
BEFORE INSERT ON users
FOR EACH ROW
BEGIN
    IF CHAR_LENGTH(TRIM(NEW.username)) = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'username must not be empty';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_users_before_update$$
CREATE TRIGGER trg_users_before_update
BEFORE UPDATE ON users
FOR EACH ROW
BEGIN
    IF CHAR_LENGTH(TRIM(NEW.username)) = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'username must not be empty';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_products_before_insert$$
CREATE TRIGGER trg_products_before_insert
BEFORE INSERT ON products
FOR EACH ROW
BEGIN
    IF CHAR_LENGTH(TRIM(NEW.title)) = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'product title must not be empty';
    END IF;
    IF NEW.original_price_cents = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'product price must be greater than zero';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_products_before_update$$
CREATE TRIGGER trg_products_before_update
BEFORE UPDATE ON products
FOR EACH ROW
BEGIN
    IF CHAR_LENGTH(TRIM(NEW.title)) = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'product title must not be empty';
    END IF;
    IF NEW.original_price_cents = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'product price must be greater than zero';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_activities_before_insert$$
CREATE TRIGGER trg_activities_before_insert
BEFORE INSERT ON flash_sale_activities
FOR EACH ROW
BEGIN
    IF NEW.flash_price_cents = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'flash price must be greater than zero';
    END IF;
    IF NEW.total_stock = 0 OR NEW.available_stock > NEW.total_stock THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'invalid flash-sale stock';
    END IF;
    IF NEW.start_time >= NEW.end_time THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'activity start_time must be before end_time';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_activities_before_update$$
CREATE TRIGGER trg_activities_before_update
BEFORE UPDATE ON flash_sale_activities
FOR EACH ROW
BEGIN
    IF NEW.flash_price_cents = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'flash price must be greater than zero';
    END IF;
    IF NEW.total_stock = 0 OR NEW.available_stock > NEW.total_stock THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'invalid flash-sale stock';
    END IF;
    IF NEW.start_time >= NEW.end_time THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'activity start_time must be before end_time';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_orders_before_insert$$
CREATE TRIGGER trg_orders_before_insert
BEFORE INSERT ON orders
FOR EACH ROW
BEGIN
    IF NEW.purchase_price_cents = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'purchase price must be greater than zero';
    END IF;
    IF NEW.quantity <> 1 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'first version only supports quantity one';
    END IF;
    IF NEW.total_amount_cents <> NEW.purchase_price_cents * NEW.quantity THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'invalid order total amount';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_orders_before_update$$
CREATE TRIGGER trg_orders_before_update
BEFORE UPDATE ON orders
FOR EACH ROW
BEGIN
    IF NEW.purchase_price_cents = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'purchase price must be greater than zero';
    END IF;
    IF NEW.quantity <> 1 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'first version only supports quantity one';
    END IF;
    IF NEW.total_amount_cents <> NEW.purchase_price_cents * NEW.quantity THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'invalid order total amount';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_inventory_logs_before_insert$$
CREATE TRIGGER trg_inventory_logs_before_insert
BEFORE INSERT ON inventory_logs
FOR EACH ROW
BEGIN
    IF NEW.change_amount = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'inventory change amount must not be zero';
    END IF;
END$$

DROP TRIGGER IF EXISTS trg_inventory_logs_before_update$$
CREATE TRIGGER trg_inventory_logs_before_update
BEFORE UPDATE ON inventory_logs
FOR EACH ROW
BEGIN
    IF NEW.change_amount = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'inventory change amount must not be zero';
    END IF;
END$$

DELIMITER ;
