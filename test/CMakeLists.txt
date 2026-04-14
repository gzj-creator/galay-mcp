cmake_minimum_required(VERSION 3.20)

# 扫描所有 .cc 文件
file(GLOB TEST_SOURCES "*.cc")

# 为每个 .cc 文件创建可执行文件
foreach(TEST_SOURCE ${TEST_SOURCES})
    # 获取文件名（不含扩展名）
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # 创建可执行文件
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # 链接 galay-mcp
    target_link_libraries(${TEST_NAME} PRIVATE galay-mcp)

    # 设置输出目录
    set_target_properties(${TEST_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()

if(BUILD_TESTING AND TARGET T5-http_client_awaitable_surface)
    add_test(
        NAME galay-mcp-http-client-awaitable-surface
        COMMAND $<TARGET_FILE:T5-http_client_awaitable_surface>
    )
    set_tests_properties(galay-mcp-http-client-awaitable-surface PROPERTIES
        LABELS "surface;compile"
    )
endif()

if(BUILD_TESTING)
    add_test(
        NAME galay-mcp-stdio-protocol-suite
        COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/S2-Run.sh"
    )
    set_tests_properties(galay-mcp-stdio-protocol-suite PROPERTIES
        ENVIRONMENT "BUILD_DIR=${CMAKE_BINARY_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        LABELS "stdio;protocol"
    )

    add_test(
        NAME galay-mcp-stdio-integration-suite
        COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/S4-RunIntegrationTest.sh"
    )
    set_tests_properties(galay-mcp-stdio-integration-suite PROPERTIES
        ENVIRONMENT "BUILD_DIR=${CMAKE_BINARY_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        LABELS "stdio;integration"
        TIMEOUT 30
    )

    add_test(
        NAME galay-mcp-http-integration-suite
        COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/S7-RunHttpIntegrationTest.sh"
    )
    set_tests_properties(galay-mcp-http-integration-suite PROPERTIES
        ENVIRONMENT "BUILD_DIR=${CMAKE_BINARY_DIR};HOST=127.0.0.1;PORT=8080"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        LABELS "http;integration"
        TIMEOUT 30
    )
endif()
