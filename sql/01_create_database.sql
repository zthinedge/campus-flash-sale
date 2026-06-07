-- Run this file once with a MySQL administrator account.
-- Replace the example password before executing it.

CREATE DATABASE IF NOT EXISTS campus_flash_sale
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS 'campus_app'@'127.0.0.1'
    IDENTIFIED BY 'change_me_before_running';

GRANT SELECT, INSERT, UPDATE, DELETE
    ON campus_flash_sale.*
    TO 'campus_app'@'127.0.0.1';

FLUSH PRIVILEGES;

SHOW GRANTS FOR 'campus_app'@'127.0.0.1';
