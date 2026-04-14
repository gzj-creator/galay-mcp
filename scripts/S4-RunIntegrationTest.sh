#!/bin/bash

set -e

BUILD_DIR="${BUILD_DIR:-build}"
SERVER_BIN="${BUILD_DIR}/bin/T2-stdio_server"
CLIENT_BIN="${BUILD_DIR}/bin/T1-stdio_client"

# 创建命名管道
PIPE_C2S="/tmp/mcp_client_to_server_$$"
PIPE_S2C="/tmp/mcp_server_to_client_$$"
SERVER_LOG="/tmp/mcp_stdio_server_$$.log"
CLIENT_LOG="/tmp/mcp_stdio_client_$$.log"

# 清理函数
cleanup() {
    rm -f "$PIPE_C2S" "$PIPE_S2C" "$SERVER_LOG" "$CLIENT_LOG"
    if [ -n "${SERVER_PID:-}" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
    fi
    if [ -n "${CLIENT_PID:-}" ]; then
        kill "$CLIENT_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

# 创建命名管道
mkfifo "$PIPE_C2S"
mkfifo "$PIPE_S2C"

echo "Starting MCP integration test..."

if [ ! -x "$SERVER_BIN" ] || [ ! -x "$CLIENT_BIN" ]; then
    echo "Missing test binaries:"
    echo "  server: $SERVER_BIN"
    echo "  client: $CLIENT_BIN"
    exit 1
fi

# 启动服务器（从client_to_server读取，向server_to_client写入）
"$SERVER_BIN" < "$PIPE_C2S" > "$PIPE_S2C" 2>"$SERVER_LOG" &
SERVER_PID=$!

# 等待服务器启动
sleep 1

# 启动客户端（向client_to_server写入，从server_to_client读取）
"$CLIENT_BIN" > "$PIPE_C2S" < "$PIPE_S2C" 2>"$CLIENT_LOG" &
CLIENT_PID=$!

# 等待客户端完成
set +e
wait $CLIENT_PID
CLIENT_EXIT=$?
set -e

# 停止服务器
kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

echo ""
if [ $CLIENT_EXIT -eq 0 ]; then
    echo "✓ Integration test passed!"
    exit 0
else
    echo "✗ Integration test failed with exit code $CLIENT_EXIT"
    if [ -s "$SERVER_LOG" ]; then
        echo "--- server log ---"
        cat "$SERVER_LOG"
    fi
    if [ -s "$CLIENT_LOG" ]; then
        echo "--- client log ---"
        cat "$CLIENT_LOG"
    fi
    exit 1
fi
