# JsonRPC 库源码构建（非 Header-only 模式）

set(JSONRPC_SOURCE_FILES
    client.cpp
    client_session.cpp
    method_registry.cpp
    protocol.cpp
    server.cpp
    server_session.cpp
    types.cpp
)

add_library(jsonrpc ${JSONRPC_SOURCE_FILES})

target_include_directories(jsonrpc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(jsonrpc
    PUBLIC
        ${Boost_LIBRARIES}
)
