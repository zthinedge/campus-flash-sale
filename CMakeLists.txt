cmake_minimum_required(VERSION 3.16)

project(campus_flash_sale
    VERSION 0.1.0
    DESCRIPTION "High-concurrency campus flash sale system"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(campus_flash_sale
    src/main.cpp
)

target_include_directories(campus_flash_sale
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
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
