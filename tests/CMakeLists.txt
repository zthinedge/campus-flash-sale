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

if(DEFINED CAMPUS_TEST_CONFIG)
    add_executable(transaction_test
        transaction_test.cpp
    )

    target_link_libraries(transaction_test
        PRIVATE
            campus_core
    )

    add_test(
        NAME transaction_raii_mysql
        COMMAND transaction_test "${CAMPUS_TEST_CONFIG}"
    )

    add_executable(dao_integration_test
        dao_integration_test.cpp
    )

    target_link_libraries(dao_integration_test
        PRIVATE
            campus_core
    )

    add_test(
        NAME dao_mysql_integration
        COMMAND dao_integration_test "${CAMPUS_TEST_CONFIG}"
    )
endif()
