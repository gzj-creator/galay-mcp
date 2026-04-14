# galay-mcp 构建选项配置

# 设置默认安装目录为/usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH "Install path prefix" FORCE)
endif()

# 构建目标选项
set(_galay_mcp_build_testing_provided FALSE)
if(DEFINED BUILD_TESTING)
    set(_galay_mcp_build_testing_provided TRUE)
endif()

option(BUILD_TESTS "Build test executables (deprecated compatibility alias for BUILD_TESTING)" ON)
if(NOT _galay_mcp_build_testing_provided)
    set(BUILD_TESTING "${BUILD_TESTS}" CACHE BOOL "Build tests with CTest" FORCE)
endif()
include(CTest)

if(_galay_mcp_build_testing_provided AND NOT BUILD_TESTS STREQUAL BUILD_TESTING)
    message(DEPRECATION
        "BUILD_TESTS is deprecated and ignored when BUILD_TESTING is explicitly set. "
        "Please use BUILD_TESTING only.")
endif()

set(BUILD_TESTS "${BUILD_TESTING}" CACHE BOOL
    "Build test executables (deprecated compatibility alias for BUILD_TESTING)" FORCE)
unset(_galay_mcp_build_testing_provided)

option(BUILD_BENCHMARKS "Build benchmark executables" ON)
option(BUILD_EXAMPLES "Build example executables" ON)
option(BUILD_MODULE_EXAMPLES "Build C++23 module(import/export) examples" ON)

# 库类型选项（CMake 标准变量）
option(BUILD_SHARED_LIBS "Build shared library instead of static" ON)

# IO后端选项
option(DISABLE_IOURING "Disable io_uring and use epoll on Linux" ON)

# 平台特定的异步IO后端检测
function(configure_io_backend)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(DISABLE_IOURING)
            message(STATUS "io_uring disabled by user, using epoll")
            add_compile_definitions(USE_EPOLL)
            set(GALAY_MCP_BACKEND "epoll" PARENT_SCOPE)
        else()
            find_library(URING_LIB uring)
            if(URING_LIB)
                message(STATUS "Found liburing: ${URING_LIB}, using io_uring")
                add_compile_definitions(USE_IOURING)
                set(GALAY_MCP_BACKEND "io_uring" PARENT_SCOPE)
                set(PLATFORM_LIBS ${URING_LIB} PARENT_SCOPE)
            else()
                message(STATUS "liburing not found, falling back to epoll")
                add_compile_definitions(USE_EPOLL)
                set(GALAY_MCP_BACKEND "epoll" PARENT_SCOPE)
            endif()
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(STATUS "macOS detected, using kqueue")
        add_compile_definitions(USE_KQUEUE)
        set(GALAY_MCP_BACKEND "kqueue" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "Windows detected, using IOCP")
        add_compile_definitions(USE_IOCP)
        set(GALAY_MCP_BACKEND "iocp" PARENT_SCOPE)
    else()
        message(WARNING "Unknown platform: ${CMAKE_SYSTEM_NAME}")
        set(GALAY_MCP_BACKEND "unknown" PARENT_SCOPE)
    endif()
endfunction()
