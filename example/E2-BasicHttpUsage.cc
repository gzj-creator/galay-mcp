/**
 * @file E2-BasicHttpUsage.cc
 * @brief 基础HTTP MCP使用示例
 * @details 演示如何使用McpHttpClient和McpHttpServer进行基本的MCP通信
 */

#include "galay-mcp/client/McpHttpClient.h"
#include "galay-mcp/server/McpHttpServer.h"
#include "galay-kernel/kernel/Runtime.h"
#include <iostream>
#include <chrono>

using namespace galay::mcp;
using namespace galay::kernel;

/**
 * @brief 简单的HTTP服务器示例
 *
 * 创建一个HTTP MCP服务器，提供基本的工具和资源
 */
void runHttpServer() {
    // 创建服务器（监听 0.0.0.0:8080）
    McpHttpServer server("0.0.0.0", 8080);

    // 设置服务器信息
    server.setServerInfo("example-http-server", "1.0.0");

    // 添加一个简单的计算器工具
    server.addTool(
        "calculate",
        "执行基本的数学计算",
        Json::object({
            {"type", "object"},
            {"properties", Json::object({
                {"operation", Json::object({
                    {"type", "string"},
                    {"enum", Json::array({"add", "subtract", "multiply", "divide"})},
                    {"description", "运算类型"}
                })},
                {"a", Json::object({
                    {"type", "number"},
                    {"description", "第一个操作数"}
                })},
                {"b", Json::object({
                    {"type", "number"},
                    {"description", "第二个操作数"}
                })}
            })},
            {"required", Json::array({"operation", "a", "b"})}
        }),
        [](const Json& args, std::expected<Json, McpError>& result) -> Coroutine {
            if (!args.contains("operation") || !args.contains("a") || !args.contains("b")) {
                result = std::unexpected(McpError(
                    McpErrorCode::InvalidParams,
                    "Missing required parameters"
                ));
                co_return;
            }

            std::string op = args["operation"];
            double a = args["a"];
            double b = args["b"];
            double answer = 0;

            if (op == "add") {
                answer = a + b;
            } else if (op == "subtract") {
                answer = a - b;
            } else if (op == "multiply") {
                answer = a * b;
            } else if (op == "divide") {
                if (b == 0) {
                    result = std::unexpected(McpError(
                        McpErrorCode::InvalidParams,
                        "Division by zero"
                    ));
                    co_return;
                }
                answer = a / b;
            } else {
                result = std::unexpected(McpError(
                    McpErrorCode::InvalidParams,
                    "Invalid operation"
                ));
                co_return;
            }

            Json res;
            res["result"] = answer;
            res["operation"] = op;
            result = res;
            co_return;
        }
    );

    // 添加一个时间资源
    server.addResource(
        "example://time",
        "current-time",
        "获取当前时间",
        "text/plain",
        [](const std::string& uri, std::expected<std::string, McpError>& result) -> Coroutine {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            result = std::ctime(&time_t);
            co_return;
        }
    );

    // 添加一个提示
    server.addPrompt(
        "code_review",
        "生成代码审查提示",
        {Json::object({
            {"name", "language"},
            {"description", "编程语言"},
            {"required", true}
        })},
        [](const std::string& name, const Json& args, std::expected<Json, McpError>& result) -> Coroutine {
            if (!args.contains("language")) {
                result = std::unexpected(McpError(
                    McpErrorCode::InvalidParams,
                    "Missing 'language' parameter"
                ));
                co_return;
            }

            std::string lang = args["language"];
            Json res;
            res["description"] = "Code review prompt for " + lang;
            res["messages"] = Json::array({
                Json::object({
                    {"role", "user"},
                    {"content", "Please review this " + lang + " code for best practices and potential issues."}
                })
            });
            result = res;
            co_return;
        }
    );

    // 启动服务器
    std::cout << "HTTP MCP Server starting on http://0.0.0.0:8080/mcp" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    server.start();
}

/**
 * @brief 简单的HTTP客户端示例
 *
 * 创建一个HTTP MCP客户端，连接到服务器并调用功能
 */
void runHttpClient(const std::string& url) {
    // 创建Runtime
    Runtime runtime(LoadBalanceStrategy::ROUND_ROBIN, 1, 1);
    runtime.start();

    // 创建客户端
    McpHttpClient client(runtime);

    // 连接到服务器
    std::cout << "Connecting to " << url << "..." << std::endl;
    auto connectResult = client.connect(url);
    if (!connectResult) {
        std::cerr << "Failed to connect: " << connectResult.error().toString() << std::endl;
        runtime.stop();
        return;
    }

    // 初始化
    std::cout << "Initializing..." << std::endl;
    auto initResult = client.initialize("example-http-client", "1.0.0");
    if (!initResult) {
        std::cerr << "Failed to initialize: " << initResult.error().toString() << std::endl;
        runtime.stop();
        return;
    }

    std::cout << "Connected to: " << client.getServerInfo().name << std::endl;

    // 列出工具
    std::cout << "\n=== Available Tools ===" << std::endl;
    auto toolsResult = client.listTools();
    if (toolsResult) {
        for (const auto& tool : toolsResult.value()) {
            std::cout << "  - " << tool.name << ": " << tool.description << std::endl;
        }
    }

    // 调用计算器工具
    std::cout << "\n=== Calling Calculator Tool ===" << std::endl;
    Json calcArgs;
    calcArgs["operation"] = "multiply";
    calcArgs["a"] = 12;
    calcArgs["b"] = 8;
    auto calcResult = client.callTool("calculate", calcArgs);
    if (calcResult) {
        std::cout << "12 * 8 = " << calcResult.value()["result"] << std::endl;
    }

    // 列出资源
    std::cout << "\n=== Available Resources ===" << std::endl;
    auto resourcesResult = client.listResources();
    if (resourcesResult) {
        for (const auto& resource : resourcesResult.value()) {
            std::cout << "  - " << resource.uri << ": " << resource.name << std::endl;
        }
    }

    // 读取时间资源
    std::cout << "\n=== Reading Time Resource ===" << std::endl;
    auto timeResult = client.readResource("example://time");
    if (timeResult) {
        std::cout << "Current time: " << timeResult.value();
    }

    // 列出提示
    std::cout << "\n=== Available Prompts ===" << std::endl;
    auto promptsResult = client.listPrompts();
    if (promptsResult) {
        for (const auto& prompt : promptsResult.value()) {
            std::cout << "  - " << prompt.name << ": " << prompt.description << std::endl;
        }
    }

    // 获取提示
    std::cout << "\n=== Getting Code Review Prompt ===" << std::endl;
    Json promptArgs;
    promptArgs["language"] = "C++";
    auto promptResult = client.getPrompt("code_review", promptArgs);
    if (promptResult) {
        std::cout << "Prompt: " << promptResult.value().dump(2) << std::endl;
    }

    // 测试ping
    std::cout << "\n=== Testing Ping ===" << std::endl;
    auto pingResult = client.ping();
    if (pingResult) {
        std::cout << "Ping successful!" << std::endl;
    }

    // 断开连接
    client.disconnect();
    runtime.stop();
    std::cout << "\nClient disconnected." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " server              - Run as server" << std::endl;
        std::cout << "  " << argv[0] << " client [url]        - Run as client" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  Terminal 1: " << argv[0] << " server" << std::endl;
        std::cout << "  Terminal 2: " << argv[0] << " client http://127.0.0.1:8080/mcp" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        runHttpServer();
    } else if (mode == "client") {
        std::string url = "http://127.0.0.1:8080/mcp";
        if (argc > 2) {
            url = argv[2];
        }
        runHttpClient(url);
    } else {
        std::cerr << "Invalid mode: " << mode << std::endl;
        std::cerr << "Use 'server' or 'client'" << std::endl;
        return 1;
    }

    return 0;
}
