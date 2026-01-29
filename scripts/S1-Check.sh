#!/bin/bash

# MCP 测试结果检查脚本
# 该脚本运行所有测试并验证结果

set -e

echo "=== MCP Test Result Checker ==="
echo ""

# 检查构建目录
if [ ! -d "build" ]; then
    echo "✗ Build directory not found. Please run 'mkdir build && cd build && cmake .. && make' first."
    exit 1
fi

cd build

# 检查可执行文件
if [ ! -f "bin/T2-StdioServer" ]; then
    echo "✗ T2-StdioServer not found. Please build the project first."
    exit 1
fi

if [ ! -f "bin/T1-StdioClient" ]; then
    echo "✗ T1-StdioClient not found. Please build the project first."
    exit 1
fi

echo "✓ Build artifacts found"
echo ""

# 运行测试脚本
if [ -f "../scripts/S2-Run.sh" ]; then
    echo "Running test suite..."
    if bash ../scripts/S2-Run.sh; then
        echo ""
        echo "✓ All tests passed successfully!"
        exit 0
    else
        echo ""
        echo "✗ Some tests failed!"
        exit 1
    fi
else
    echo "✗ Test script not found at ../scripts/S2-Run.sh"
    exit 1
fi
