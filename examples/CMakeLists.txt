cmake_minimum_required(VERSION 3.20)

# 查找 galay-http 和 galay-kernel
find_library(GALAY_HTTP_LIB NAMES galay-http PATHS /usr/local/lib /usr/lib)
find_library(GALAY_KERNEL_LIB NAMES galay-kernel PATHS /usr/local/lib /usr/lib)

function(add_mcp_example target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE galay-mcp)

    if(target_name MATCHES ".*[Hh]ttp.*" AND GALAY_HTTP_LIB AND GALAY_KERNEL_LIB)
        target_link_libraries(${target_name} PRIVATE ${GALAY_HTTP_LIB} ${GALAY_KERNEL_LIB})
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endfunction()

function(add_mcp_import_example target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE galay-mcp-modules)

    if(target_name MATCHES ".*[Hh]ttp.*" AND GALAY_HTTP_LIB AND GALAY_KERNEL_LIB)
        target_link_libraries(${target_name} PRIVATE ${GALAY_HTTP_LIB} ${GALAY_KERNEL_LIB})
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        CXX_SCAN_FOR_MODULES ON
    )
endfunction()

if(BUILD_EXAMPLES)
    # include 版本
    add_mcp_example(E1-BasicStdioUsage include/E1-BasicStdioUsage.cc)
    add_mcp_example(E2-BasicHttpUsage include/E2-BasicHttpUsage.cc)

    # import 版本（按工具链支持自动开启）
    if(BUILD_MODULE_EXAMPLES AND TARGET galay-mcp-modules)
        add_mcp_import_example(E1-BasicStdioUsageImport import/E1-BasicStdioUsage.cc)
        add_mcp_import_example(E2-BasicHttpUsageImport import/E2-BasicHttpUsage.cc)
    endif()
endif()
