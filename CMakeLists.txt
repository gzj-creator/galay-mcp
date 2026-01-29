cmake_minimum_required(VERSION 3.20)

project(galay-mcp
    VERSION 1.0.0
    DESCRIPTION "MCP (Model Context Protocol) implementation based on stdio"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加子目录
add_subdirectory(galay-mcp)

# 可选：构建测试
option(BUILD_TESTS "Build test programs" ON)
if(BUILD_TESTS)
    add_subdirectory(test)
endif()

# 可选：构建性能测试
option(BUILD_BENCHMARKS "Build benchmark programs" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# 可选：构建示例
option(BUILD_EXAMPLES "Build example programs" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(example)
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