cmake_minimum_required(VERSION 3.20)

project(auction LANGUAGES CXX)

set(QT_INSTALLROOT "../qt-6.0.1-dynamic-msvc2019-x86_64")
list(APPEND CMAKE_PREFIX_PATH ${QT_INSTALLROOT}"/lib/cmake")

#set(OPENSSL_ROOT_DIR ${QT_INSTALLROOT}"/openssl")
set(OPENSSL_INCLUDE_DIR ${QT_INSTALLROOT}"/openssl")
set(OPENSSL_CRYPTO_LIBRARY ${QT_INSTALLROOT}"/lib/libcrypto.lib")
set(OPENSSL_LIBRARIES ${QT_INSTALLROOT}"/lib/libssl.lib" ${QT_INSTALLROOT}"/lib/libcrypto.lib")
set(WrapOpenSSL_FOUND ON)

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

add_custom_command(TARGET auction POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::Core> $<TARGET_FILE_DIR:auction>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::Network> $<TARGET_FILE_DIR:auction>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::Sql> $<TARGET_FILE_DIR:auction>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/${QT_INSTALLROOT}/bin/libssl-1_1-x64.dll" $<TARGET_FILE_DIR:auction>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/${QT_INSTALLROOT}/bin/libcrypto-1_1-x64.dll" $<TARGET_FILE_DIR:auction>
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/${QT_INSTALLROOT}/plugins" $<TARGET_FILE_DIR:auction>
    )
