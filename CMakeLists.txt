cmake_minimum_required(VERSION 3.20)

project(galay-mcp
    VERSION 1.0.0
    DESCRIPTION "MCP (Model Context Protocol) implementation based on stdio"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 可选构建项
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_BENCHMARKS "Build benchmark programs" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_MODULE_EXAMPLES "Build C++23 module(import/export) examples" ON)

# C++23 命名模块支持能力检测
set(GALAY_MCP_MODULES_SUPPORTED TRUE)
set(GALAY_MCP_MODULES_DISABLE_REASON "")

if(CMAKE_VERSION VERSION_LESS 3.28)
    set(GALAY_MCP_MODULES_SUPPORTED FALSE)
    set(GALAY_MCP_MODULES_DISABLE_REASON "CMake >= 3.28 is required")
endif()

if(GALAY_MCP_MODULES_SUPPORTED)
    if(NOT (CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Visual Studio"))
        set(GALAY_MCP_MODULES_SUPPORTED FALSE)
        set(GALAY_MCP_MODULES_DISABLE_REASON
            "generator '${CMAKE_GENERATOR}' does not support C++ module scanning")
    endif()
endif()

if(GALAY_MCP_MODULES_SUPPORTED AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_program(GALAY_MCP_CLANG_SCAN_DEPS_EXECUTABLE NAMES clang-scan-deps)
    if(NOT GALAY_MCP_CLANG_SCAN_DEPS_EXECUTABLE)
        get_filename_component(_GALAY_MCP_CLANG_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(GALAY_MCP_CLANG_SCAN_DEPS_EXECUTABLE NAMES clang-scan-deps HINTS "${_GALAY_MCP_CLANG_BIN_DIR}")
    endif()
    if(NOT GALAY_MCP_CLANG_SCAN_DEPS_EXECUTABLE)
        set(GALAY_MCP_MODULES_SUPPORTED FALSE)
        set(GALAY_MCP_MODULES_DISABLE_REASON "clang-scan-deps not found")
    endif()
endif()

if(BUILD_MODULE_EXAMPLES AND NOT GALAY_MCP_MODULES_SUPPORTED)
    message(WARNING
        "BUILD_MODULE_EXAMPLES requested but disabled: ${GALAY_MCP_MODULES_DISABLE_REASON}")
    set(BUILD_MODULE_EXAMPLES OFF CACHE BOOL "Build C++23 module(import/export) examples" FORCE)
endif()

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加子目录
add_subdirectory(galay-mcp)

# 可选：构建测试
if(BUILD_TESTS)
    add_subdirectory(test)
endif()

# 可选：构建性能测试
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# 可选：构建示例
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 生成 CMake 配置文件供其他项目使用
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mcp-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/galay-mcp-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mcp-config.cmake"
    INSTALL_DESTINATION lib/cmake/galay-mcp
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mcp-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-mcp-config-version.cmake"
    DESTINATION lib/cmake/galay-mcp
)

install(EXPORT galay-mcp-targets
    FILE galay-mcp-config.cmake
    DESTINATION lib/cmake/galay-mcp
)


install(EXPORT galay-mcp-targets
    FILE GalayMcpConfig.cmake
    DESTINATION lib/cmake/galay-mcp
)

install(EXPORT galay-mcp-targets
    FILE galaymcp-config.cmake
    DESTINATION lib/cmake/galay-mcp
)
