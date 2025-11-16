# JsonRPC 单元测试

cmake_minimum_required(VERSION 3.10)

# 查找 Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 包含项目头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# 定义测试可执行文件
set(TEST_SOURCES
    test_types.cpp
    test_protocol.cpp
    test_type_converter.cpp
    test_server.cpp
    test_client.cpp
    test_integration.cpp
)

# 创建测试可执行文件
add_executable(jsonrpc_tests ${TEST_SOURCES})

# 链接库
target_link_libraries(jsonrpc_tests
    ${GTEST_BOTH_LIBRARIES}
    pthread
    jsonrpc
)

# 添加测试
enable_testing()
add_test(NAME jsonrpc_tests COMMAND jsonrpc_tests)
