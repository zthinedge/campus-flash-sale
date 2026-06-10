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
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_path(MYSQL_INCLUDE_DIR
    NAMES mysql/mysql.h
    REQUIRED
)
find_library(MYSQLCLIENT_LIBRARY
    NAMES mysqlclient
    REQUIRED
)

add_library(campus_core STATIC
    src/app/CliApp.cpp
    src/app/WebApp.cpp
    src/dao/ActivityDao.cpp
    src/dao/InventoryLogDao.cpp
    src/dao/OrderDao.cpp
    src/dao/ProductDao.cpp
    src/dao/UserDao.cpp
    src/db/MySqlConnection.cpp
    src/db/MySqlError.cpp
    src/db/PreparedStatement.cpp
    src/db/Transaction.cpp
    src/http/HttpRequest.cpp
    src/http/HttpResponse.cpp
    src/http/HttpServer.cpp
    src/model/Enums.cpp
    src/net/Acceptor.cpp
    src/net/Buffer.cpp
    src/net/Channel.cpp
    src/net/Epoll.cpp
    src/net/EventLoop.cpp
    src/net/InetAddress.cpp
    src/net/Socket.cpp
    src/net/TcpConnection.cpp
    src/net/TcpServer.cpp
    src/service/ActivityService.cpp
    src/service/AdminService.cpp
    src/service/AuthService.cpp
    src/service/FlashSaleService.cpp
    src/service/OrderService.cpp
    src/service/ProductService.cpp
    src/service/ServiceError.cpp
    src/utils/Config.cpp
    src/utils/PasswordHasher.cpp
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
        OpenSSL::Crypto
        Threads::Threads
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

target_compile_definitions(campus_flash_sale
    PRIVATE
        CAMPUS_WEB_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/web"
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
