#!/bin/bash

# MCP 性能测试运行脚本
# 该脚本运行所有性能测试并收集结果

set -e

echo "=== MCP Benchmark Suite ==="
echo ""

# 检查构建目录
if [ ! -d "build" ]; then
    echo "✗ Build directory not found. Please run 'mkdir build && cd build && cmake .. && make' first."
    exit 1
fi

cd build

# 检查性能测试可执行文件
BENCHMARKS=(
    "B1-StdioPerformance"
    "B2-HttpPerformance"
    "B3-ConcurrentRequests"
)

echo "Checking benchmark executables..."
for BENCH in "${BENCHMARKS[@]}"; do
    if [ ! -f "bin/$BENCH" ]; then
        echo "✗ $BENCH not found. Please build the project first."
        exit 1
    fi
done
echo "✓ All benchmark executables found"
echo ""

# 运行 Stdio 性能测试
echo "=== Running Stdio Performance Test ==="
if [ -f "bin/T2-StdioServer" ]; then
    ./bin/B1-StdioPerformance | ./bin/T2-StdioServer
    echo "✓ Stdio performance test completed"
else
    echo "✗ T2-StdioServer not found, skipping Stdio benchmark"
fi
echo ""

# 运行 HTTP 性能测试（需要先启动服务器）
echo "=== Running HTTP Performance Test ==="
echo "Note: Make sure HTTP server is running on http://127.0.0.1:8080/mcp"
echo "You can start it with: ./bin/T4-HttpServer"
echo ""
read -p "Press Enter when server is ready, or Ctrl+C to skip..."

if [ -f "bin/B2-HttpPerformance" ]; then
    ./bin/B2-HttpPerformance
    echo "✓ HTTP performance test completed"
fi
echo ""

# 运行并发测试
echo "=== Running Concurrent Requests Test ==="
echo "Note: Make sure HTTP server is running on http://127.0.0.1:8080/mcp"
echo ""
read -p "Press Enter when server is ready, or Ctrl+C to skip..."

if [ -f "bin/B3-ConcurrentRequests" ]; then
    ./bin/B3-ConcurrentRequests --threads 10 --requests 100
    echo "✓ Concurrent requests test completed"
fi
echo ""

echo "=== All Benchmarks Complete ==="
echo ""
echo "Remember to save results to docs/ directory:"
echo "  - docs/B1-Stdio性能测试.md"
echo "  - docs/B2-HTTP性能测试.md"
echo "  - docs/B3-并发请求压测.md"
