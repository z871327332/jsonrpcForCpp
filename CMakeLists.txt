cmake_minimum_required(VERSION 3.10)
project(JsonRPC VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 选项：Header-Only 模式（默认开启）
option(JSONRPC_HEADER_ONLY "Build as header-only library" ON)

# 选项：构建测试
option(JSONRPC_BUILD_TESTS "Build tests" ON)

# 选项：构建示例
option(JSONRPC_BUILD_EXAMPLES "Build examples" ON)

# 设置 CMake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 查找 Boost 库（优先本地 third_party/boost/，其次系统 Boost）
# README 要求 1.83+，需启用 JSON/System 等组件
include(FindLocalBoost)
find_local_boost(1.83.0 REQUIRED COMPONENTS json system)

# Boost.JSON 源文件（避免额外依赖）
add_library(jsonrpc_boost_json STATIC src/boost_json_amalg.cpp)
target_include_directories(jsonrpc_boost_json PRIVATE ${Boost_INCLUDE_DIR})

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIR})

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX-)
    # 禁用一些 MSVC 特定的警告
    add_compile_options(/wd4100 /wd4127)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Header-Only 模式定义
if(JSONRPC_HEADER_ONLY)
    add_definitions(-DJSONRPC_HEADER_ONLY)

    # Header-Only 库目标（接口库）
    add_library(jsonrpc INTERFACE)
    target_include_directories(jsonrpc INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(jsonrpc INTERFACE jsonrpc_boost_json)
else()
    # 编译为库
    add_subdirectory(src)
    target_link_libraries(jsonrpc PUBLIC jsonrpc_boost_json)
endif()

# 构建测试
if(JSONRPC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 构建示例
if(JSONRPC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 安装规则
install(DIRECTORY include/jsonrpc DESTINATION include)

# 查找 Doxygen（用于生成文档）
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # 添加 doc 目标
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "使用 Doxygen 生成 API 文档"
        VERBATIM
    )

    message(STATUS "Doxygen 已找到，可以使用 'make doc' 生成文档")
else()
    message(STATUS "未找到 Doxygen，跳过文档生成目标")
endif()
