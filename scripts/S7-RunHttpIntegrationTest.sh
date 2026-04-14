#!/bin/bash

set -e

BUILD_DIR="${BUILD_DIR:-build}"
HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-8080}"
URL="http://${HOST}:${PORT}/mcp"
SERVER_BIN="${BUILD_DIR}/bin/T4-http_server"
CLIENT_BIN="${BUILD_DIR}/bin/T3-http_client"

cleanup() {
    if [ -n "${SERVER_PID:-}" ]; then
        kill "${SERVER_PID}" 2>/dev/null || true
        wait "${SERVER_PID}" 2>/dev/null || true
    fi
}

trap cleanup EXIT

if [ ! -x "$SERVER_BIN" ] || [ ! -x "$CLIENT_BIN" ]; then
    echo "Missing HTTP test binaries:"
    echo "  server: $SERVER_BIN"
    echo "  client: $CLIENT_BIN"
    exit 1
fi

echo "Starting HTTP MCP integration test..."
"$SERVER_BIN" "$PORT" "$HOST" >/tmp/mcp_http_server_$$.log 2>&1 &
SERVER_PID=$!

for _ in $(seq 1 50); do
    if curl -sS -m 1 -o /dev/null "$URL"; then
        break
    fi
    sleep 0.1
done

if ! curl -sS -m 1 -o /dev/null "$URL"; then
    echo "HTTP server did not become ready: $URL"
    exit 1
fi

"$CLIENT_BIN" "$URL"
echo "✓ HTTP integration test passed"
