#!/bin/bash

# HTTP MCP Server 测试脚本
# 用于测试协程版本的工具、资源和提示功能

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 服务器配置
HOST="localhost"
PORT="8080"
URL="http://${HOST}:${PORT}/mcp"

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 打印函数
print_header() {
    echo -e "\n${YELLOW}========================================${NC}"
    echo -e "${YELLOW}$1${NC}"
    echo -e "${YELLOW}========================================${NC}\n"
}

print_test() {
    echo -e "${YELLOW}[TEST $TOTAL_TESTS] $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ PASSED${NC}: $1\n"
    ((PASSED_TESTS++))
}

print_failure() {
    echo -e "${RED}✗ FAILED${NC}: $1"
    echo -e "${RED}Response: $2${NC}\n"
    ((FAILED_TESTS++))
}

# 发送JSON-RPC请求
send_request() {
    local method=$1
    local params=$2
    local id=$3

    local request="{\"jsonrpc\":\"2.0\",\"id\":${id},\"method\":\"${method}\",\"params\":${params}}"

    curl -s -X POST "${URL}" \
        -H "Content-Type: application/json" \
        -d "${request}"
}

# 测试函数
test_initialize() {
    ((TOTAL_TESTS++))
    print_test "Initialize connection"

    local params='{"protocolVersion":"2024-11-05","clientInfo":{"name":"test-client","version":"1.0.0"},"capabilities":{}}'
    local response=$(send_request "initialize" "${params}" 1)

    if echo "${response}" | grep -q '"result"' && echo "${response}" | grep -q '"serverInfo"'; then
        print_success "Server initialized successfully"
        return 0
    else
        print_failure "Initialize failed" "${response}"
        return 1
    fi
}

test_tools_list() {
    ((TOTAL_TESTS++))
    print_test "List available tools"

    local params='{}'
    local response=$(send_request "tools/list" "${params}" 2)

    if echo "${response}" | grep -q '"tools"' && echo "${response}" | grep -q '"echo"' && echo "${response}" | grep -q '"add"'; then
        print_success "Tools list retrieved (echo, add)"
        return 0
    else
        print_failure "Tools list failed" "${response}"
        return 1
    fi
}

test_echo_tool() {
    ((TOTAL_TESTS++))
    print_test "Call echo tool (coroutine)"

    local params='{"name":"echo","arguments":{"message":"Hello from coroutine"}}'
    local response=$(send_request "tools/call" "${params}" 3)

    if echo "${response}" | grep -q '"result"' && echo "${response}" | grep -q "Hello from coroutine"; then
        print_success "Echo tool executed successfully"
        return 0
    else
        print_failure "Echo tool failed" "${response}"
        return 1
    fi
}

test_add_tool() {
    ((TOTAL_TESTS++))
    print_test "Call add tool (coroutine)"

    local params='{"name":"add","arguments":{"a":15,"b":25}}'
    local response=$(send_request "tools/call" "${params}" 4)

    if echo "${response}" | grep -q '"result"' && echo "${response}" | grep -q "sum"; then
        print_success "Add tool executed successfully (15 + 25 = 40)"
        return 0
    else
        print_failure "Add tool failed" "${response}"
        return 1
    fi
}

test_resources_list() {
    ((TOTAL_TESTS++))
    print_test "List available resources"

    local params='{}'
    local response=$(send_request "resources/list" "${params}" 5)

    if echo "${response}" | grep -q '"resources"' && echo "${response}" | grep -q '"example://hello"'; then
        print_success "Resources list retrieved"
        return 0
    else
        print_failure "Resources list failed" "${response}"
        return 1
    fi
}

test_resource_read() {
    ((TOTAL_TESTS++))
    print_test "Read resource (coroutine)"

    local params='{"uri":"example://hello"}'
    local response=$(send_request "resources/read" "${params}" 6)

    if echo "${response}" | grep -q "Hello from MCP HTTP Server"; then
        print_success "Resource read successfully"
        return 0
    else
        print_failure "Resource read failed" "${response}"
        return 1
    fi
}

test_prompts_list() {
    ((TOTAL_TESTS++))
    print_test "List available prompts"

    local params='{}'
    local response=$(send_request "prompts/list" "${params}" 7)

    if echo "${response}" | grep -q '"prompts"' && echo "${response}" | grep -q '"greeting"'; then
        print_success "Prompts list retrieved"
        return 0
    else
        print_failure "Prompts list failed" "${response}"
        return 1
    fi
}

test_prompt_get() {
    ((TOTAL_TESTS++))
    print_test "Get prompt (coroutine)"

    local params='{"name":"greeting","arguments":{"name":"Alice"}}'
    local response=$(send_request "prompts/get" "${params}" 8)

    if echo "${response}" | grep -q "Hello, Alice"; then
        print_success "Prompt retrieved successfully"
        return 0
    else
        print_failure "Prompt get failed" "${response}"
        return 1
    fi
}

test_ping() {
    ((TOTAL_TESTS++))
    print_test "Ping server"

    local params='{}'
    local response=$(send_request "ping" "${params}" 9)

    if echo "${response}" | grep -q '"result"'; then
        print_success "Server responded to ping"
        return 0
    else
        print_failure "Ping failed" "${response}"
        return 1
    fi
}

# 主测试流程
main() {
    print_header "HTTP MCP Server Coroutine Test Suite"

    echo "Testing server at: ${URL}"
    echo ""

    # 检查服务器是否运行
    if ! curl -s -o /dev/null -w "%{http_code}" "${URL}" > /dev/null 2>&1; then
        echo -e "${RED}Error: Server is not running at ${URL}${NC}"
        echo "Please start the server first: ./bin/test_http_server ${PORT}"
        exit 1
    fi

    # 运行所有测试
    test_initialize
    test_tools_list
    test_echo_tool
    test_add_tool
    test_resources_list
    test_resource_read
    test_prompts_list
    test_prompt_get
    test_ping

    # 打印测试结果
    print_header "Test Results"
    echo "Total Tests:  ${TOTAL_TESTS}"
    echo -e "${GREEN}Passed:       ${PASSED_TESTS}${NC}"

    if [ ${FAILED_TESTS} -gt 0 ]; then
        echo -e "${RED}Failed:       ${FAILED_TESTS}${NC}"
        exit 1
    else
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    fi
}

# 运行主函数
main
