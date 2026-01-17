cmake_minimum_required(VERSION 3.20)

# 添加测试可执行文件
add_executable(test_stdio_server test_stdio_server.cc)
target_link_libraries(test_stdio_server PRIVATE galay-mcp)

add_executable(test_stdio_client test_stdio_client.cc)
target_link_libraries(test_stdio_client PRIVATE galay-mcp)

# 设置输出目录
set_target_properties(test_stdio_server test_stdio_client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
