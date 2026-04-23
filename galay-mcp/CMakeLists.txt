cmake_minimum_required(VERSION 3.20)

project(galay-mcp VERSION 1.1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(simdjson CONFIG QUIET)
if(NOT TARGET simdjson::simdjson)
    find_path(SIMDJSON_INCLUDE_DIR simdjson.h
        PATHS /opt/homebrew/include /usr/local/include)
    find_library(SIMDJSON_LIBRARY NAMES simdjson
        PATHS /opt/homebrew/lib /usr/local/lib)

    if(NOT SIMDJSON_INCLUDE_DIR OR NOT SIMDJSON_LIBRARY)
        message(FATAL_ERROR "simdjson not found, please install it")
    endif()

    add_library(simdjson::simdjson UNKNOWN IMPORTED)
    set_target_properties(simdjson::simdjson PROPERTIES
        IMPORTED_LOCATION "${SIMDJSON_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SIMDJSON_INCLUDE_DIR}"
    )
endif()

find_package(galay-kernel 3.4.4 CONFIG REQUIRED)
find_package(GalayHttp 2.0.2 CONFIG REQUIRED)

# 收集源文件
file(GLOB_RECURSE SRC_LIST
    "common/*.cc"
    "client/*.cc"
    "server/*.cc"
    "*.cc"
)

# 创建库
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

# C++23 module 版本（仅在根 CMake 允许时构建）
if(BUILD_MODULE_EXAMPLES)
    add_library(${PROJECT_NAME}-modules STATIC)
    target_link_libraries(${PROJECT_NAME}-modules PUBLIC ${PROJECT_NAME})
    target_include_directories(${PROJECT_NAME}-modules PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
    )
    target_sources(${PROJECT_NAME}-modules
        PUBLIC
            FILE_SET CXX_MODULES
            FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/module/galay.mcp.cppm"
    )

    set_target_properties(${PROJECT_NAME}-modules PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
endif()

# 设置编译选项
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# 包含目录
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

# 链接依赖
target_link_libraries(${PROJECT_NAME} PUBLIC
    simdjson::simdjson
    GalayHttp::galay-http
    galay-kernel::galay-kernel
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    EXPORT galay-mcp-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

if(TARGET ${PROJECT_NAME}-modules)
    install(TARGETS ${PROJECT_NAME}-modules
        EXPORT galay-mcp-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        FILE_SET CXX_MODULES DESTINATION include/galay-mcp/module
    )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/galay-mcp
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.cppm"
    PATTERN "*.inl"
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "*.cc" EXCLUDE
)
