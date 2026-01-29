/**
 * @file E1-BasicStdioUsage.cc
 * @brief 基础Stdio MCP使用示例
 * @details 演示如何使用McpStdioClient和McpStdioServer进行基本的MCP通信
 */

#include "galay-mcp/client/McpStdioClient.h"
#include "galay-mcp/server/McpStdioServer.h"
#include <iostream>

using namespace galay::mcp;

/**
 * @brief 简单的服务器示例
 *
 * 创建一个MCP服务器，提供基本的工具和资源
 */
void runSimpleServer() {
    McpStdioServer server;

    // 设置服务器信息
    server.setServerInfo("example-server", "1.0.0");

    // 添加一个简单的echo工具
    server.addTool(
        "echo",
        "回显输入的消息",
        Json::object({
            {"type", "object"},
            {"properties", Json::object({
                {"message", Json::object({
                    {"type", "string"},
                    {"description", "要回显的消息"}
                })}
            })},
            {"required", Json::array({"message"})}
        }),
        [](const Json& args) -> std::expected<Json, McpError> {
            if (!args.contains("message")) {
                return std::unexpected(McpError(
                    McpErrorCode::InvalidParams,
                    "Missing 'message' parameter"
                ));
            }

            Json result;
            result["echo"] = args["message"];
            return result;
        }
    );

    // 添加一个简单的资源
    server.addResource(
        "example://greeting",
        "greeting",
        "简单的问候资源",
        "text/plain",
        [](const std::string& uri) -> std::expected<std::string, McpError> {
            return "Hello from MCP Server!";
        }
    );

    // 运行服务器（阻塞）
    std::cerr << "Server started. Waiting for requests..." << std::endl;
    server.run();
}

/**
 * @brief 简单的客户端示例
 *
 * 创建一个MCP客户端，连接到服务器并调用功能
 */
void runSimpleClient() {
    McpStdioClient client;

    // 初始化连接
    std::cout << "Initializing client..." << std::endl;
    auto initResult = client.initialize("example-client", "1.0.0");
    if (!initResult) {
        std::cerr << "Failed to initialize: " << initResult.error().toString() << std::endl;
        return;
    }

    std::cout << "Connected to server: " << client.getServerInfo().name << std::endl;

    // 列出可用工具
    std::cout << "\nListing tools..." << std::endl;
    auto toolsResult = client.listTools();
    if (toolsResult) {
        for (const auto& tool : toolsResult.value()) {
            std::cout << "  - " << tool.name << ": " << tool.description << std::endl;
        }
    }

    // 调用echo工具
    std::cout << "\nCalling echo tool..." << std::endl;
    Json args;
    args["message"] = "Hello, MCP!";
    auto callResult = client.callTool("echo", args);
    if (callResult) {
        std::cout << "Result: " << callResult.value().dump(2) << std::endl;
    }

    // 列出资源
    std::cout << "\nListing resources..." << std::endl;
    auto resourcesResult = client.listResources();
    if (resourcesResult) {
        for (const auto& resource : resourcesResult.value()) {
            std::cout << "  - " << resource.uri << ": " << resource.name << std::endl;
        }
    }

    // 读取资源
    std::cout << "\nReading resource..." << std::endl;
    auto readResult = client.readResource("example://greeting");
    if (readResult) {
        std::cout << "Content: " << readResult.value() << std::endl;
    }

    // 断开连接
    client.disconnect();
    std::cout << "\nClient disconnected." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " server  - Run as server" << std::endl;
        std::cout << "  " << argv[0] << " client  - Run as client" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  Terminal 1: " << argv[0] << " server" << std::endl;
        std::cout << "  Terminal 2: " << argv[0] << " client | " << argv[0] << " server" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        runSimpleServer();
    } else if (mode == "client") {
        runSimpleClient();
    } else {
        std::cerr << "Invalid mode: " << mode << std::endl;
        std::cerr << "Use 'server' or 'client'" << std::endl;
        return 1;
    }

    return 0;
}
