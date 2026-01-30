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

// 性能测试协程
Coroutine runBenchmark(McpHttpClient& client, const std::string& url, int& exitCode) {
    // 连接到服务器
    std::cout << "\nConnecting to server..." << std::endl;
    auto connectResult = co_await client.connect(url);
    if (!connectResult) {
        std::cerr << "Failed to connect: " << connectResult.error().message() << std::endl;
        exitCode = 1;
        co_return;
    }

    // 初始化
    std::cout << "Initializing client..." << std::endl;
    std::expected<void, McpError> initResult;
    co_await client.initialize("benchmark-http-client", "1.0.0", initResult).wait();
    if (!initResult) {
        std::cerr << "Failed to initialize: " << initResult.error().message() << std::endl;
        exitCode = 1;
        co_return;
    }
    std::cout << "Connected to: " << client.getServerInfo().name << std::endl;

    const size_t iterations = 1000;

    // Ping测试
    {
        PerformanceStats stats;
        std::cout << "\nBenchmarking ping (" << iterations << " iterations)..." << std::endl;

        for (size_t i = 0; i < iterations; ++i) {
            auto start = high_resolution_clock::now();
            std::expected<void, McpError> pingResult;
            co_await client.ping(pingResult).wait();
            auto end = high_resolution_clock::now();

            if (pingResult) {
                double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                stats.addLatency(latencyMs);
            }

            if ((i + 1) % 100 == 0) {
                std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
            }
        }
        stats.printReport("HTTP Ping");
    }

    // Tool Call测试
    {
        PerformanceStats stats;
        std::cout << "\nBenchmarking tool calls (" << iterations << " iterations)..." << std::endl;

        for (size_t i = 0; i < iterations; ++i) {
            Json args;
            args["message"] = "Benchmark test message " + std::to_string(i);

            auto start = high_resolution_clock::now();
            std::expected<Json, McpError> callResult;
            co_await client.callTool("echo", args, callResult).wait();
            auto end = high_resolution_clock::now();

            if (callResult) {
                double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                stats.addLatency(latencyMs);
            }

            if ((i + 1) % 100 == 0) {
                std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
            }
        }
        stats.printReport("HTTP Tool Call");
    }

    // Resource Read测试
    {
        PerformanceStats stats;
        std::cout << "\nBenchmarking resource reads (" << iterations << " iterations)..." << std::endl;

        for (size_t i = 0; i < iterations; ++i) {
            auto start = high_resolution_clock::now();
            std::expected<std::string, McpError> readResult;
            co_await client.readResource("example://hello", readResult).wait();
            auto end = high_resolution_clock::now();

            if (readResult) {
                double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                stats.addLatency(latencyMs);
            }

            if ((i + 1) % 100 == 0) {
                std::cout << "Progress: " << (i + 1) << "/" << iterations << "\r" << std::flush;
            }
        }
        stats.printReport("HTTP Resource Read");
    }

    // List Operations测试
    {
        PerformanceStats toolsStats, resourcesStats, promptsStats;
        std::cout << "\nBenchmarking list operations (" << iterations << " iterations)..." << std::endl;

        for (size_t i = 0; i < iterations; ++i) {
            {
                auto start = high_resolution_clock::now();
                std::expected<std::vector<Tool>, McpError> result;
                co_await client.listTools(result).wait();
                auto end = high_resolution_clock::now();
                if (result) {
                    double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                    toolsStats.addLatency(latencyMs);
                }
            }

            {
                auto start = high_resolution_clock::now();
                std::expected<std::vector<Resource>, McpError> result;
                co_await client.listResources(result).wait();
                auto end = high_resolution_clock::now();
                if (result) {
                    double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
                    resourcesStats.addLatency(latencyMs);
                }
            }

            {
                auto start = high_resolution_clock::now();
                std::expected<std::vector<Prompt>, McpError> result;
                co_await client.listPrompts(result).wait();
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

    // 断开连接
    co_await client.disconnect();
    exitCode = 0;
    co_return;
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

    int exitCode = 0;

    // 创建Runtime
    Runtime runtime(LoadBalanceStrategy::ROUND_ROBIN, 1, 1);
    runtime.start();

    // 创建客户端
    McpHttpClient client(runtime);

    // 在IO调度器上运行测试协程
    auto* scheduler = runtime.getNextIOScheduler();
    scheduler->spawn(runBenchmark(client, url, exitCode));

    // 等待测试完成
    std::this_thread::sleep_for(std::chrono::seconds(60));

    runtime.stop();

    std::cout << "\n=== Benchmark Complete ===" << std::endl;

    return exitCode;
}
