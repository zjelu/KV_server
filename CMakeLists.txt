cmake_minimum_required(VERSION 3.16)

project(KV_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加 googletest
add_subdirectory(
    third_party/googletest
)


enable_testing()

#添加Asan检查内存泄漏
add_compile_options(
    -fsanitize=address
    -fsanitize=undefined
)

add_link_options(
    -fsanitize=address
    -fsanitize=undefined
)

add_executable(
    kvtest

    tests/KVStore_test.cpp
    tests/parser_test.cpp
    tests/aof_test.cpp

    src/KVStore.cpp
    src/parser.cpp
    src/AOFlog.cpp
    src/executor.cpp
)


target_include_directories(
    kvtest
    PRIVATE
    include
)


target_link_libraries(
    kvtest
    GTest::gtest_main
)


include(GoogleTest)

gtest_discover_tests(kvtest)


add_executable(kvserver
    src/main.cpp
    src/connstate.cpp
    src/executor.cpp
    src/KVStore.cpp
    src/parser.cpp
    src/AOFlog.cpp
    src/KVserver.cpp
)

target_include_directories(kvserver
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(kvserver
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(kvclient
    client.cpp
)

target_compile_options(kvclient
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)