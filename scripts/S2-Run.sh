#!/bin/bash

# MCP Server 测试脚本
# 该脚本测试MCP服务器的各项功能

set -e

SERVER="./bin/T2-StdioServer"
TEMP_OUTPUT="/tmp/mcp_test_output_$$"
TEMP_INPUT="/tmp/mcp_test_input_$$"

# 清理函数
cleanup() {
    rm -f "$TEMP_OUTPUT" "$TEMP_INPUT"
}

trap cleanup EXIT

echo "=== MCP Server Test Suite ==="
echo ""

# 测试1: 初始化
echo "Test 1: Initialize"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | head -1 > "$TEMP_OUTPUT"

if grep -q '"result"' "$TEMP_OUTPUT" && grep -q '"serverInfo"' "$TEMP_OUTPUT"; then
    echo "✓ Initialize test passed"
else
    echo "✗ Initialize test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

# 测试2: 工具列表
echo "Test 2: List Tools"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | tail -1 > "$TEMP_OUTPUT"

if grep -q '"tools"' "$TEMP_OUTPUT" && grep -q '"add"' "$TEMP_OUTPUT"; then
    echo "✓ List tools test passed"
else
    echo "✗ List tools test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

# 测试3: 调用工具
echo "Test 3: Call Tool (add)"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"add","arguments":{"a":10,"b":20}}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | tail -1 > "$TEMP_OUTPUT"

if grep -q '"result"' "$TEMP_OUTPUT" && grep -q '30' "$TEMP_OUTPUT"; then
    echo "✓ Call tool test passed"
else
    echo "✗ Call tool test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

# 测试4: 资源列表
echo "Test 4: List Resources"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
{"jsonrpc":"2.0","id":2,"method":"resources/list","params":{}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | tail -1 > "$TEMP_OUTPUT"

if grep -q '"resources"' "$TEMP_OUTPUT" && grep -q 'test.txt' "$TEMP_OUTPUT"; then
    echo "✓ List resources test passed"
else
    echo "✗ List resources test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

# 测试5: 读取资源
echo "Test 5: Read Resource"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
{"jsonrpc":"2.0","id":2,"method":"resources/read","params":{"uri":"file:///test.txt"}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | tail -1 > "$TEMP_OUTPUT"

if grep -q '"contents"' "$TEMP_OUTPUT" && grep -q 'test file content' "$TEMP_OUTPUT"; then
    echo "✓ Read resource test passed"
else
    echo "✗ Read resource test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

# 测试6: 提示列表
echo "Test 6: List Prompts"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
{"jsonrpc":"2.0","id":2,"method":"prompts/list","params":{}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | tail -1 > "$TEMP_OUTPUT"

if grep -q '"prompts"' "$TEMP_OUTPUT" && grep -q 'write_essay' "$TEMP_OUTPUT"; then
    echo "✓ List prompts test passed"
else
    echo "✗ List prompts test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

# 测试7: Ping
echo "Test 7: Ping"
cat > "$TEMP_INPUT" << 'EOF'
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"test","version":"1.0"},"capabilities":{}}}
{"jsonrpc":"2.0","id":2,"method":"ping","params":{}}
EOF
cat "$TEMP_INPUT" | $SERVER 2>/dev/null | tail -1 > "$TEMP_OUTPUT"

if grep -q '"id":2' "$TEMP_OUTPUT" && grep -q '"result"' "$TEMP_OUTPUT"; then
    echo "✓ Ping test passed"
else
    echo "✗ Ping test failed"
    cat "$TEMP_OUTPUT"
    exit 1
fi

echo ""
echo "=== All tests passed! ==="
exit 0
