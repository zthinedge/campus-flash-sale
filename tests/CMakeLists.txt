add_test(
    NAME campus_flash_sale_startup
    COMMAND campus_flash_sale
)

set_tests_properties(campus_flash_sale_startup PROPERTIES
    PASS_REGULAR_EXPRESSION "campus-flash-sale starting"
)
