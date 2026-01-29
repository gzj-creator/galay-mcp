/**
 * @file test_http_client.cc
 * @brief HTTP MCP Client 测试示例
 * @details 演示如何使用 McpHttpClient 连接到HTTP MCP服务器并调用功能
 */

#include "galay-mcp/client/McpHttpClient.h"
#include "galay-kernel/kernel/Runtime.h"
#include <iostream>
#include <string>

using namespace galay::mcp;
using namespace galay::kernel;

void printSeparator() {
    std::cout << "========================================\n";
}

void printError(const McpError& error) {
    std::cerr << "Error: " << error.message() << "\n";
    if (!error.details().empty()) {
        std::cerr << "Details: " << error.details() << "\n";
    }
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string url = "http://127.0.0.1:8080/mcp";

    if (argc > 1) {
        url = argv[1];
    }

    printSeparator();
    std::cout << "HTTP MCP Client Test\n";
    printSeparator();
    std::cout << "Server URL: " << url << "\n";
    printSeparator();
    std::cout << "\n";

    try {
        // 创建Runtime
        Runtime runtime(LoadBalanceStrategy::ROUND_ROBIN, 1, 1);
        runtime.start();
        std::cout << "Runtime started\n\n";

        // 创建客户端
        McpHttpClient client(runtime);

        // 连接到服务器
        std::cout << "Connecting to server...\n";
        auto connectResult = client.connect(url);
        if (!connectResult) {
            printError(connectResult.error());
            return 1;
        }
        std::cout << "Connected successfully\n\n";

        // 初始化
        std::cout << "Initializing...\n";
        auto initResult = client.initialize("test-http-client", "1.0.0");
        if (!initResult) {
            printError(initResult.error());
            return 1;
        }
        std::cout << "Initialized successfully\n";

        auto serverInfo = client.getServerInfo();
        std::cout << "Server: " << serverInfo.name << " v" << serverInfo.version << "\n\n";

        // 测试ping
        printSeparator();
        std::cout << "Testing ping...\n";
        auto pingResult = client.ping();
        if (pingResult) {
            std::cout << "Ping successful\n";
        } else {
            printError(pingResult.error());
        }
        std::cout << "\n";

        // 列出工具
        printSeparator();
        std::cout << "Listing tools...\n";
        auto toolsResult = client.listTools();
        if (toolsResult) {
            std::cout << "Available tools:\n";
            for (const auto& tool : toolsResult.value()) {
                std::cout << "  - " << tool.name << ": " << tool.description << "\n";
            }
        } else {
            printError(toolsResult.error());
        }
        std::cout << "\n";

        // 调用echo工具
        printSeparator();
        std::cout << "Calling echo tool...\n";
        Json echoArgs;
        echoArgs["message"] = "Hello from HTTP client!";
        auto echoResult = client.callTool("echo", echoArgs);
        if (echoResult) {
            std::cout << "Echo result: " << echoResult.value().dump(2) << "\n";
        } else {
            printError(echoResult.error());
        }
        std::cout << "\n";

        // 调用add工具
        printSeparator();
        std::cout << "Calling add tool...\n";
        Json addArgs;
        addArgs["a"] = 42;
        addArgs["b"] = 58;
        auto addResult = client.callTool("add", addArgs);
        if (addResult) {
            std::cout << "Add result: " << addResult.value().dump(2) << "\n";
        } else {
            printError(addResult.error());
        }
        std::cout << "\n";

        // 列出资源
        printSeparator();
        std::cout << "Listing resources...\n";
        auto resourcesResult = client.listResources();
        if (resourcesResult) {
            std::cout << "Available resources:\n";
            for (const auto& resource : resourcesResult.value()) {
                std::cout << "  - " << resource.uri << ": " << resource.name << "\n";
            }
        } else {
            printError(resourcesResult.error());
        }
        std::cout << "\n";

        // 读取资源
        printSeparator();
        std::cout << "Reading resource...\n";
        auto readResult = client.readResource("example://hello");
        if (readResult) {
            std::cout << "Resource content: " << readResult.value() << "\n";
        } else {
            printError(readResult.error());
        }
        std::cout << "\n";

        // 列出提示
        printSeparator();
        std::cout << "Listing prompts...\n";
        auto promptsResult = client.listPrompts();
        if (promptsResult) {
            std::cout << "Available prompts:\n";
            for (const auto& prompt : promptsResult.value()) {
                std::cout << "  - " << prompt.name << ": " << prompt.description << "\n";
            }
        } else {
            printError(promptsResult.error());
        }
        std::cout << "\n";

        // 获取提示
        printSeparator();
        std::cout << "Getting prompt...\n";
        Json promptArgs;
        promptArgs["name"] = "Alice";
        auto promptResult = client.getPrompt("greeting", promptArgs);
        if (promptResult) {
            std::cout << "Prompt result: " << promptResult.value().dump(2) << "\n";
        } else {
            printError(promptResult.error());
        }
        std::cout << "\n";

        // 断开连接
        printSeparator();
        std::cout << "Disconnecting...\n";
        client.disconnect();
        std::cout << "Disconnected\n\n";

        // 停止Runtime
        runtime.stop();
        std::cout << "Runtime stopped\n";

        printSeparator();
        std::cout << "All tests completed successfully!\n";
        printSeparator();

    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
