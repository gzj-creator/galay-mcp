cmake_minimum_required(VERSION 3.20)

# 查找 galay-http 和 galay-kernel
find_library(GALAY_HTTP_LIB NAMES galay-http PATHS /usr/local/lib /usr/lib)
find_library(GALAY_KERNEL_LIB NAMES galay-kernel PATHS /usr/local/lib /usr/lib)

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

    # 如果是 HTTP 测试，链接 galay-http 和 galay-kernel
    if(TEST_NAME MATCHES ".*http.*" AND GALAY_HTTP_LIB AND GALAY_KERNEL_LIB)
        target_link_libraries(${TEST_NAME} PRIVATE ${GALAY_HTTP_LIB} ${GALAY_KERNEL_LIB})
    endif()

    # 设置输出目录
    set_target_properties(${TEST_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()
