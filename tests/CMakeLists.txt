add_test(
    NAME campus_flash_sale_startup
    COMMAND campus_flash_sale
        --check-config
        "${PROJECT_SOURCE_DIR}/config/config.example.json"
)

set_tests_properties(campus_flash_sale_startup PROPERTIES
    PASS_REGULAR_EXPRESSION "config validation successful"
)

add_executable(config_test
    config_test.cpp
)

target_link_libraries(config_test
    PRIVATE
        campus_core
)

target_compile_definitions(config_test
    PRIVATE
        CAMPUS_CONFIG_EXAMPLE_PATH="${PROJECT_SOURCE_DIR}/config/config.example.json"
)

add_test(
    NAME config_load_example
    COMMAND config_test
)
