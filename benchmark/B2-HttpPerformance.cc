/**
 * @file B2-HttpPerformance.cc
 * @brief HTTP MCP性能测试
 * @details 测试基于HTTP的MCP协议性能，包括吞吐量、延迟等指标
 */

#include "galay-mcp/client/McpHttpClient.h"
#include "galay-kernel/kernel/Runtime.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <cmath>
#include <iomanip>

using namespace galay::mcp;
using namespace galay::kernel;
using namespace std::chrono;

// 性能统计结构
struct PerformanceStats {
    size_t totalRequests = 0;
    double totalTimeMs = 0.0;
    double minLatencyMs = std::numeric_limits<double>::max();
    double maxLatencyMs = 0.0;
    std::vector<double> latencies;

    void addLatency(double latencyMs) {
        latencies.push_back(latencyMs);
        totalTimeMs += latencyMs;
        minLatencyMs = std::min(minLatencyMs, latencyMs);
        maxLatencyMs = std::max(maxLatencyMs, latencyMs);
        totalRequests++;
    }

    double getAvgLatencyMs() const {
        return totalRequests > 0 ? totalTimeMs / totalRequests : 0.0;
    }

    double getMedianLatencyMs() {
        if (latencies.empty()) return 0.0;
        std::vector<double> sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        size_t mid = sorted.size() / 2;
        if (sorted.size() % 2 == 0) {
            return (sorted[mid - 1] + sorted[mid]) / 2.0;
        }
        return sorted[mid];
    }

    double getP95LatencyMs() {
        if (latencies.empty()) return 0.0;
        std::vector<double> sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = static_cast<size_t>(sorted.size() * 0.95);
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    double getP99LatencyMs() {
        if (latencies.empty()) return 0.0;
        std::vector<double> sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = static_cast<size_t>(sorted.size() * 0.99);
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    double getStdDevMs() const {
        if (latencies.size() < 2) return 0.0;
        double avg = getAvgLatencyMs();
        double sumSquares = 0.0;
        for (double lat : latencies) {
            double diff = lat - avg;
            sumSquares += diff * diff;
        }
        return std::sqrt(sumSquares / latencies.size());
    }

    void printReport(const std::string& testName) const {
        std::cout << "\n=== " << testName << " Performance Report ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total Requests:  " << totalRequests << std::endl;
        std::cout << "Total Time:      " << totalTimeMs << " ms" << std::endl;
        std::cout << "Avg Latency:     " << const_cast<PerformanceStats*>(this)->getAvgLatencyMs() << " ms" << std::endl;
        std::cout << "Median Latency:  " << const_cast<PerformanceStats*>(this)->getMedianLatencyMs() << " ms" << std::endl;
        std::cout << "Min Latency:     " << minLatencyMs << " ms" << std::endl;
        std::cout << "Max Latency:     " << maxLatencyMs << " ms" << std::endl;
        std::cout << "P95 Latency:     " << const_cast<PerformanceStats*>(this)->getP95LatencyMs() << " ms" << std::endl;
        std::cout << "P99 Latency:     " << const_cast<PerformanceStats*>(this)->getP99LatencyMs() << " ms" << std::endl;
        std::cout << "Std Dev:         " << getStdDevMs() << " ms" << std::endl;
        if (totalTimeMs > 0) {
            std::cout << "Throughput:      " << (totalRequests * 1000.0 / totalTimeMs) << " req/s" << std::endl;
        }
    }
};

// 测试工具调用性能
void benchmarkToolCall(McpHttpClient& client, size_t iterations) {
    PerformanceStats stats;

    std::cout << "\nBenchmarking tool calls (" << iterations << " iterations)..." << std::endl;

    for (size_t i = 0; i < iterations; ++i) {
        Json args;
        args["message"] = "Benchmark test message " + std::to_string(i);

        auto start = high_resolution_clock::now();
        auto result = client.callTool("echo", args);
        auto end = high_resolution_clock::now();

        if (!result) {
            std::cerr << "Error in iteration " << i << ": " << result.error().toString() << std::endl;
            continue;
        }

        double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
        stats.addLatency(latencyMs);

        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
        }
    }

    stats.printReport("HTTP Tool Call");
}

// 测试资源读取性能
void benchmarkResourceRead(McpHttpClient& client, size_t iterations) {
    PerformanceStats stats;

    std::cout << "\nBenchmarking resource reads (" << iterations << " iterations)..." << std::endl;

    for (size_t i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        auto result = client.readResource("example://hello");
        auto end = high_resolution_clock::now();

        if (!result) {
            std::cerr << "Error in iteration " << i << ": " << result.error().toString() << std::endl;
            continue;
        }

        double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
        stats.addLatency(latencyMs);

        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
        }
    }

    stats.printReport("HTTP Resource Read");
}

// 测试列表操作性能
void benchmarkListOperations(McpHttpClient& client, size_t iterations) {
    PerformanceStats toolsStats;
    PerformanceStats resourcesStats;
    PerformanceStats promptsStats;

    std::cout << "\nBenchmarking list operations (" << iterations << " iterations)..." << std::endl;

    for (size_t i = 0; i < iterations; ++i) {
        {
            auto start = high_resolution_clock::now();
            auto result = client.listTools();
            auto end = high_resolution_clock::now();
            if (result) {
                double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                toolsStats.addLatency(latencyMs);
            }
        }

        {
            auto start = high_resolution_clock::now();
            auto result = client.listResources();
            auto end = high_resolution_clock::now();
            if (result) {
                double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                resourcesStats.addLatency(latencyMs);
            }
        }

        {
            auto start = high_resolution_clock::now();
            auto result = client.listPrompts();
            auto end = high_resolution_clock::now();
            if (result) {
                double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                promptsStats.addLatency(latencyMs);
            }
        }

        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
        }
    }

    toolsStats.printReport("HTTP List Tools");
    resourcesStats.printReport("HTTP List Resources");
    promptsStats.printReport("HTTP List Prompts");
}

// 测试Ping性能
void benchmarkPing(McpHttpClient& client, size_t iterations) {
    PerformanceStats stats;

    std::cout << "\nBenchmarking ping (" << iterations << " iterations)..." << std::endl;

    for (size_t i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        auto result = client.ping();
        auto end = high_resolution_clock::now();

        if (!result) {
            std::cerr << "Error in iteration " << i << ": " << result.error().toString() << std::endl;
            continue;
        }

        double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
        stats.addLatency(latencyMs);

        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
        }
    }

    stats.printReport("HTTP Ping");
}

void printSystemInfo() {
    std::cout << "\n=== System Information ===" << std::endl;
    std::cout << "Test Date: " << __DATE__ << " " << __TIME__ << std::endl;

    #ifdef __APPLE__
    std::cout << "Platform: macOS" << std::endl;
    #elif __linux__
    std::cout << "Platform: Linux" << std::endl;
    #else
    std::cout << "Platform: Unknown" << std::endl;
    #endif

    std::cout << "Compiler: " << __VERSION__ << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;
}

int main(int argc, char* argv[]) {
    std::string url = "http://127.0.0.1:8080/mcp";
    if (argc > 1) {
        url = argv[1];
    }

    printSystemInfo();

    std::cout << "\n=== HTTP MCP Performance Benchmark ===" << std::endl;
    std::cout << "Server URL: " << url << std::endl;
    std::cout << "Make sure the HTTP MCP server is running!" << std::endl;

    // 创建Runtime
    Runtime runtime(LoadBalanceStrategy::ROUND_ROBIN, 1, 1);
    runtime.start();

    // 创建客户端
    McpHttpClient client(runtime);

    // 连接到服务器
    std::cout << "\nConnecting to server..." << std::endl;
    auto connectResult = client.connect(url);
    if (!connectResult) {
        std::cerr << "Failed to connect: " << connectResult.error().toString() << std::endl;
        runtime.stop();
        return 1;
    }

    // 初始化
    std::cout << "Initializing client..." << std::endl;
    auto initResult = client.initialize("benchmark-http-client", "1.0.0");
    if (!initResult) {
        std::cerr << "Failed to initialize: " << initResult.error().toString() << std::endl;
        runtime.stop();
        return 1;
    }
    std::cout << "Connected to: " << client.getServerInfo().name << std::endl;

    // 运行各项性能测试
    const size_t iterations = 1000;

    benchmarkPing(client, iterations);
    benchmarkToolCall(client, iterations);
    benchmarkResourceRead(client, iterations);
    benchmarkListOperations(client, iterations);

    // 断开连接
    client.disconnect();
    runtime.stop();

    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    std::cout << "\nNote: Save these results to docs/B2-HTTP性能测试.md" << std::endl;

    return 0;
}
