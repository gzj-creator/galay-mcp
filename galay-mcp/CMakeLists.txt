cmake_minimum_required(VERSION 3.20)

project(galay-mcp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_path(SIMDJSON_INCLUDE_DIR simdjson.h
    PATHS /opt/homebrew/include /usr/local/include)
find_library(SIMDJSON_LIBRARY NAMES simdjson
    PATHS /opt/homebrew/lib /usr/local/lib)

if(NOT SIMDJSON_INCLUDE_DIR OR NOT SIMDJSON_LIBRARY)
    message(FATAL_ERROR "simdjson not found, please install it")
endif()

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
        ${SIMDJSON_INCLUDE_DIR}
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

# 查找 galay-http 和 galay-kernel（可选）
find_library(GALAY_HTTP_LIB NAMES galay-http PATHS /usr/local/lib /usr/lib)
find_library(GALAY_KERNEL_LIB NAMES galay-kernel PATHS /usr/local/lib /usr/lib)

# 包含目录
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
    ${SIMDJSON_INCLUDE_DIR}
)

# 链接依赖
target_link_libraries(${PROJECT_NAME} PUBLIC ${SIMDJSON_LIBRARY})

# 如果找到 galay-http 和 galay-kernel，添加其头文件和库
if(GALAY_HTTP_LIB AND GALAY_KERNEL_LIB)
    target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/include)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${GALAY_HTTP_LIB} ${GALAY_KERNEL_LIB})
endif()

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
