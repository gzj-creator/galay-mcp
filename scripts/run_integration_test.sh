#!/bin/bash

# 创建命名管道
PIPE_C2S="/tmp/mcp_client_to_server_$$"
PIPE_S2C="/tmp/mcp_server_to_client_$$"

# 清理函数
cleanup() {
    rm -f "$PIPE_C2S" "$PIPE_S2C"
    kill $SERVER_PID 2>/dev/null
    kill $CLIENT_PID 2>/dev/null
}

trap cleanup EXIT

# 创建命名管道
mkfifo "$PIPE_C2S"
mkfifo "$PIPE_S2C"

echo "Starting MCP integration test..."

# 启动服务器（从client_to_server读取，向server_to_client写入）
./bin/test_stdio_server < "$PIPE_C2S" > "$PIPE_S2C" 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 1

# 启动客户端（向client_to_server写入，从server_to_client读取）
./bin/test_stdio_client > "$PIPE_C2S" < "$PIPE_S2C" 2>&1 &
CLIENT_PID=$!

# 等待客户端完成
wait $CLIENT_PID
CLIENT_EXIT=$?

# 停止服务器
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
if [ $CLIENT_EXIT -eq 0 ]; then
    echo "✓ Integration test passed!"
    exit 0
else
    echo "✗ Integration test failed with exit code $CLIENT_EXIT"
    exit 1
fi
