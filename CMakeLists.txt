cmake_minimum_required(VERSION 3.14)

project(auction LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Network Sql REQUIRED)

add_executable(auction
    src/auction/main.cpp
    src/auction/vendor.h
    src/auction/config.h

    src/auction/db.h src/auction/db.cpp  src/auction/db_res.h
    src/auction/server.h src/auction/server.cpp
    src/auction/session.h src/auction/session.cpp
    src/auction/ui.h src/auction/ui.cpp
)

target_link_libraries(auction Qt6::Core Qt6::Network Qt6::Sql )
