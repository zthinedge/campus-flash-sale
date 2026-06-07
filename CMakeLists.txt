cmake_minimum_required(VERSION 3.16)

project(campus_flash_sale
    VERSION 0.1.0
    DESCRIPTION "High-concurrency campus flash sale system"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json 3.10 REQUIRED)
find_path(MYSQL_INCLUDE_DIR
    NAMES mysql/mysql.h
    REQUIRED
)
find_library(MYSQLCLIENT_LIBRARY
    NAMES mysqlclient
    REQUIRED
)

add_library(campus_core STATIC
    src/db/MySqlConnection.cpp
    src/utils/Config.cpp
)

target_include_directories(campus_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${MYSQL_INCLUDE_DIR}"
)

target_link_libraries(campus_core
    PUBLIC
        "${MYSQLCLIENT_LIBRARY}"
        nlohmann_json::nlohmann_json
)

target_compile_options(campus_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(campus_flash_sale
    src/main.cpp
)

target_link_libraries(campus_flash_sale
    PRIVATE
        campus_core
)

target_compile_options(campus_flash_sale
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
