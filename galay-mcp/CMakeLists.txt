cmake_minimum_required(VERSION 3.20)

project(galay-mcp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(nlohmann_json 3.2.0 QUIET)

# 检查 nlohmann_json 是否找到，如果没有则查找 header-only 版本
set(NLOHMANN_JSON_INCLUDE_DIR "")
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json package not found, searching for header-only version")
    if(EXISTS "/opt/homebrew/include/nlohmann/json.hpp")
        set(NLOHMANN_JSON_INCLUDE_DIR "/opt/homebrew/include")
        message(STATUS "Found nlohmann/json at /opt/homebrew/include")
    elseif(EXISTS "/usr/local/include/nlohmann/json.hpp")
        set(NLOHMANN_JSON_INCLUDE_DIR "/usr/local/include")
        message(STATUS "Found nlohmann/json at /usr/local/include")
    else()
        message(FATAL_ERROR "nlohmann/json not found, please install it")
    endif()
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

# 链接依赖
if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
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

# 如果使用 header-only 版本的 nlohmann_json，添加其包含目录
if(NOT nlohmann_json_FOUND AND NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PUBLIC ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    EXPORT galay-mcp-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/galay-mcp
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "*.cc" EXCLUDE
)
