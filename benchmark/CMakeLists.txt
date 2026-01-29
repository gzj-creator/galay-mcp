cmake_minimum_required(VERSION 3.20)

# 查找 galay-http 和 galay-kernel
find_library(GALAY_HTTP_LIB NAMES galay-http PATHS /usr/local/lib /usr/lib)
find_library(GALAY_KERNEL_LIB NAMES galay-kernel PATHS /usr/local/lib /usr/lib)

# 扫描所有 .cc 文件
file(GLOB BENCHMARK_SOURCES "*.cc")

# 为每个 .cc 文件创建可执行文件
foreach(BENCHMARK_SOURCE ${BENCHMARK_SOURCES})
    # 获取文件名（不含扩展名）
    get_filename_component(BENCHMARK_NAME ${BENCHMARK_SOURCE} NAME_WE)

    # 创建可执行文件
    add_executable(${BENCHMARK_NAME} ${BENCHMARK_SOURCE})

    # 链接 galay-mcp
    target_link_libraries(${BENCHMARK_NAME} PRIVATE galay-mcp)

    # 如果是 HTTP 相关的压测，链接 galay-http 和 galay-kernel
    if(BENCHMARK_NAME MATCHES ".*[Hh]ttp.*" AND GALAY_HTTP_LIB AND GALAY_KERNEL_LIB)
        target_link_libraries(${BENCHMARK_NAME} PRIVATE ${GALAY_HTTP_LIB} ${GALAY_KERNEL_LIB})
    endif()

    # 如果是并发测试，链接 galay-http 和 galay-kernel
    if(BENCHMARK_NAME MATCHES ".*[Cc]oncurrent.*" AND GALAY_HTTP_LIB AND GALAY_KERNEL_LIB)
        target_link_libraries(${BENCHMARK_NAME} PRIVATE ${GALAY_HTTP_LIB} ${GALAY_KERNEL_LIB})
    endif()

    # 设置输出目录
    set_target_properties(${BENCHMARK_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()
