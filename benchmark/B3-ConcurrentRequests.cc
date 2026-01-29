/**
 * @file B3-ConcurrentRequests.cc
 * @brief 并发请求压测
 * @details 测试MCP服务器在高并发场景下的性能表现
 */

#include "galay-mcp/client/McpHttpClient.h"
#include "galay-kernel/kernel/Runtime.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>
#include <iomanip>

using namespace galay::mcp;
using namespace galay::kernel;
using namespace std::chrono;

// 线程安全的性能统计
class ConcurrentStats {
public:
    void addLatency(double latencyMs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latencies.push_back(latencyMs);
        m_totalTimeMs += latencyMs;
        m_minLatencyMs = std::min(m_minLatencyMs, latencyMs);
        m_maxLatencyMs = std::max(m_maxLatencyMs, latencyMs);
        m_totalRequests++;
    }

    void addError() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorCount++;
    }

    void printReport(const std::string& testName, double totalTestTimeMs) {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::cout << "\n=== " << testName << " Concurrent Performance Report ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total Requests:    " << m_totalRequests << std::endl;
        std::cout << "Successful:        " << m_totalRequests << std::endl;
        std::cout << "Failed:            " << m_errorCount << std::endl;
        std::cout << "Success Rate:      " << (m_totalRequests * 100.0 / (m_totalRequests + m_errorCount)) << "%" << std::endl;
        std::cout << "Test Duration:     " << totalTestTimeMs << " ms" << std::endl;

        if (m_totalRequests > 0) {
            double avgLatency = m_totalTimeMs / m_totalRequests;
            std::cout << "Avg Latency:       " << avgLatency << " ms" << std::endl;
            std::cout << "Min Latency:       " << m_minLatencyMs << " ms" << std::endl;
            std::cout << "Max Latency:       " << m_maxLatencyMs << " ms" << std::endl;

            // 计算中位数和百分位
            std::vector<double> sorted = m_latencies;
            std::sort(sorted.begin(), sorted.end());

            size_t mid = sorted.size() / 2;
            double median = sorted.size() % 2 == 0
                ? (sorted[mid - 1] + sorted[mid]) / 2.0
                : sorted[mid];
            std::cout << "Median Latency:    " << median << " ms" << std::endl;

            size_t p95Idx = static_cast<size_t>(sorted.size() * 0.95);
            std::cout << "P95 Latency:       " << sorted[std::min(p95Idx, sorted.size() - 1)] << " ms" << std::endl;

            size_t p99Idx = static_cast<size_t>(sorted.size() * 0.99);
            std::cout << "P99 Latency:       " << sorted[std::min(p99Idx, sorted.size() - 1)] << " ms" << std::endl;

            // 计算标准差
            double sumSquares = 0.0;
            for (double lat : m_latencies) {
                double diff = lat - avgLatency;
                sumSquares += diff * diff;
            }
            double stdDev = std::sqrt(sumSquares / m_latencies.size());
            std::cout << "Std Dev:           " << stdDev << " ms" << std::endl;

            // 计算QPS
            if (totalTestTimeMs > 0) {
                double qps = (m_totalRequests + m_errorCount) * 1000.0 / totalTestTimeMs;
                std::cout << "QPS:               " << qps << " req/s" << std::endl;
            }
        }
    }

private:
    std::mutex m_mutex;
    std::vector<double> m_latencies;
    size_t m_totalRequests = 0;
    size_t m_errorCount = 0;
    double m_totalTimeMs = 0.0;
    double m_minLatencyMs = std::numeric_limits<double>::max();
    double m_maxLatencyMs = 0.0;
};

// 工作线程函数
void workerThread(const std::string& url, size_t requestsPerThread, ConcurrentStats& stats, std::atomic<bool>& ready) {
    // 等待所有线程准备就绪
    while (!ready.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 创建Runtime和客户端
    Runtime runtime(LoadBalanceStrategy::ROUND_ROBIN, 1, 1);
    runtime.start();

    McpHttpClient client(runtime);

    // 连接并初始化
    auto connectResult = client.connect(url);
    if (!connectResult) {
        stats.addError();
        runtime.stop();
        return;
    }

    auto initResult = client.initialize("concurrent-client", "1.0.0");
    if (!initResult) {
        stats.addError();
        runtime.stop();
        return;
    }

    // 执行请求
    for (size_t i = 0; i < requestsPerThread; ++i) {
        Json args;
        args["message"] = "Concurrent test " + std::to_string(i);

        auto start = high_resolution_clock::now();
        auto result = client.callTool("echo", args);
        auto end = high_resolution_clock::now();

        if (result) {
            double latencyMs = duration_cast<microseconds>(end - start).count() / 1000.0;
            stats.addLatency(latencyMs);
        } else {
            stats.addError();
        }
    }

    client.disconnect();
    runtime.stop();
}

// 并发测试
void runConcurrentTest(const std::string& url, size_t numThreads, size_t requestsPerThread) {
    std::cout << "\n=== Concurrent Test ===" << std::endl;
    std::cout << "Threads:           " << numThreads << std::endl;
    std::cout << "Requests/Thread:   " << requestsPerThread << std::endl;
    std::cout << "Total Requests:    " << (numThreads * requestsPerThread) << std::endl;
    std::cout << "\nStarting test..." << std::endl;

    ConcurrentStats stats;
    std::vector<std::thread> threads;
    std::atomic<bool> ready(false);

    // 创建所有线程
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerThread, url, requestsPerThread, std::ref(stats), std::ref(ready));
    }

    // 等待所有线程准备就绪
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 开始测试
    auto testStart = high_resolution_clock::now();
    ready.store(true);

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    auto testEnd = high_resolution_clock::now();

    double totalTestTimeMs = duration_cast<milliseconds>(testEnd - testStart).count();

    // 打印报告
    stats.printReport("Concurrent Tool Call", totalTestTimeMs);
}

// 逐步增加并发测试
void runScalabilityTest(const std::string& url) {
    std::cout << "\n=== Scalability Test ===" << std::endl;
    std::cout << "Testing with increasing concurrency levels..." << std::endl;

    std::vector<size_t> concurrencyLevels = {1, 2, 4, 8, 16, 32};
    const size_t requestsPerThread = 100;

    for (size_t numThreads : concurrencyLevels) {
        std::cout << "\n--- Testing with " << numThreads << " threads ---" << std::endl;
        runConcurrentTest(url, numThreads, requestsPerThread);

        // 短暂休息，让服务器恢复
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
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
    std::cout << "Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;
}

int main(int argc, char* argv[]) {
    std::string url = "http://127.0.0.1:8080/mcp";
    size_t numThreads = 10;
    size_t requestsPerThread = 100;
    bool scalabilityTest = false;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--url" && i + 1 < argc) {
            url = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            numThreads = std::stoul(argv[++i]);
        } else if (arg == "--requests" && i + 1 < argc) {
            requestsPerThread = std::stoul(argv[++i]);
        } else if (arg == "--scalability") {
            scalabilityTest = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --url <url>          Server URL (default: http://127.0.0.1:8080/mcp)" << std::endl;
            std::cout << "  --threads <n>        Number of concurrent threads (default: 10)" << std::endl;
            std::cout << "  --requests <n>       Requests per thread (default: 100)" << std::endl;
            std::cout << "  --scalability        Run scalability test with increasing concurrency" << std::endl;
            std::cout << "  --help               Show this help message" << std::endl;
            return 0;
        }
    }

    printSystemInfo();

    std::cout << "\n=== Concurrent Requests Benchmark ===" << std::endl;
    std::cout << "Server URL: " << url << std::endl;
    std::cout << "Make sure the HTTP MCP server is running!" << std::endl;

    if (scalabilityTest) {
        runScalabilityTest(url);
    } else {
        runConcurrentTest(url, numThreads, requestsPerThread);
    }

    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    std::cout << "\nNote: Save these results to docs/B3-并发请求压测.md" << std::endl;

    return 0;
}
