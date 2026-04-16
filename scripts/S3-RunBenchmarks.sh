#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-${BUILD_DIR:-build}}"
BIN_DIR="${BUILD_DIR}/bin"

require_bin() {
    local path="$1"
    if [ ! -x "${path}" ]; then
        echo "missing executable: ${path}" >&2
        exit 1
    fi
}

echo "=== MCP Benchmark Suite ==="
echo "build_dir: ${BUILD_DIR}"
echo

require_bin "${BIN_DIR}/B1-stdio_performance"
require_bin "${BIN_DIR}/B2-http_performance"
require_bin "${BIN_DIR}/B3-concurrent_requests"
require_bin "${BIN_DIR}/T2-stdio_server"
require_bin "${BIN_DIR}/T4-http_server"

echo "== B1 stdio benchmark =="
mkfifo /tmp/galay-mcp-b1-c2s /tmp/galay-mcp-b1-s2c
"${BIN_DIR}/T2-stdio_server" < /tmp/galay-mcp-b1-c2s > /tmp/galay-mcp-b1-s2c &
SERVER_PID=$!
trap 'kill ${SERVER_PID} >/dev/null 2>&1 || true; wait ${SERVER_PID} 2>/dev/null || true; rm -f /tmp/galay-mcp-b1-c2s /tmp/galay-mcp-b1-s2c' EXIT
"${BIN_DIR}/B1-stdio_performance" 1000 > /tmp/galay-mcp-b1-c2s < /tmp/galay-mcp-b1-s2c
kill ${SERVER_PID} >/dev/null 2>&1 || true
wait ${SERVER_PID} 2>/dev/null || true
rm -f /tmp/galay-mcp-b1-c2s /tmp/galay-mcp-b1-s2c
trap - EXIT
echo

echo "== B2/B3 HTTP benchmarks =="
echo "start server in another terminal:"
echo "  ${BIN_DIR}/T4-http_server 8080 0.0.0.0"
echo
read -r -p "Press Enter when the HTTP server is ready, or Ctrl+C to abort..."
"${BIN_DIR}/B2-http_performance" --url http://127.0.0.1:8080/mcp --connections 8 --requests 2000 --io 2 --compute 0
echo
"${BIN_DIR}/B3-concurrent_requests" --url http://127.0.0.1:8080/mcp --workers 10 --requests 100
echo

echo "=== Benchmark reminders ==="
echo "- Save raw stdout for every run."
echo "- B1 uses the local stdio server target."
echo "- B2/B3 expect a running HTTP MCP server at http://127.0.0.1:8080/mcp."
