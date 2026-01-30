/**
 * @file T4-HttpServer.cc
 * @brief HTTP MCP Server 测试示例
 */

#include "galay-mcp/server/McpHttpServer.h"
#include "galay-mcp/common/McpSchemaBuilder.h"
#include <iostream>
#include <string>
#include <signal.h>

using namespace galay::mcp;
using namespace galay::kernel;

McpHttpServer* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", stopping server...\n";
    if (g_server) {
        g_server->stop();
    }
}

// Echo工具（协程）
Coroutine echoTool(const Json& arguments, std::expected<Json, McpError>& result) {
    try {
        std::string message = arguments.value("message", "");
        Json res;
        res["echo"] = message;
        res["length"] = message.length();
        result = res;
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::invalidParams(e.what()));
    }
    co_return;
}

// 加法工具（协程）
Coroutine addTool(const Json& arguments, std::expected<Json, McpError>& result) {
    try {
        double a = arguments.value("a", 0.0);
        double b = arguments.value("b", 0.0);
        Json res;
        res["sum"] = a + b;
        result = res;
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::invalidParams(e.what()));
    }
    co_return;
}

// 资源读取器（协程）
Coroutine readExampleResource(const std::string& uri, std::expected<std::string, McpError>& result) {
    if (uri == "example://hello") {
        result = "Hello from MCP HTTP Server!";
    } else if (uri == "example://info") {
        result = "This is a test resource from the HTTP MCP server.";
    } else {
        result = std::unexpected(McpError::resourceNotFound(uri));
    }
    co_return;
}

// 提示获取器（协程）
Coroutine getExamplePrompt(const std::string& name, const Json& arguments, std::expected<Json, McpError>& result) {
    if (name == "greeting") {
        std::string userName = arguments.value("name", "User");

        Json res;
        res["description"] = "A friendly greeting";

        Json messages = Json::array();
        Json message;
        message["role"] = "user";
        message["content"] = Json::object();
        message["content"]["type"] = "text";
        message["content"]["text"] = "Hello, " + userName + "! How can I help you today?";
        messages.push_back(message);

        res["messages"] = messages;
        result = res;
    } else {
        result = std::unexpected(McpError::promptNotFound(name));
    }
    co_return;
}

int main(int argc, char* argv[]) {
    std::string host = "0.0.0.0";
    int port = 8080;

    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    if (argc > 2) {
        host = argv[2];
    }

    std::cout << "========================================\n";
    std::cout << "HTTP MCP Server Test\n";
    std::cout << "========================================\n";
    std::cout << "Server will listen on " << host << ":" << port << "\n";
    std::cout << "MCP endpoint: http://" << host << ":" << port << "/mcp\n";
    std::cout << "========================================\n\n";

    try {
        McpHttpServer server(host, port);
        g_server = &server;

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        server.setServerInfo("test-http-mcp-server", "1.0.0");

        auto echoSchema = SchemaBuilder()
            .addString("message", "The message to echo", true)
            .build();
        server.addTool("echo", "Echo back the input message", echoSchema, echoTool);

        auto addSchema = SchemaBuilder()
            .addNumber("a", "First number", true)
            .addNumber("b", "Second number", true)
            .build();
        server.addTool("add", "Add two numbers", addSchema, addTool);

        server.addResource("example://hello", "Hello Resource",
                          "A simple hello message", "text/plain",
                          readExampleResource);

        server.addResource("example://info", "Info Resource",
                          "Information about the server", "text/plain",
                          readExampleResource);

        auto promptArgs = PromptArgumentBuilder()
            .addArgument("name", "User's name", false)
            .build();
        server.addPrompt("greeting", "Generate a friendly greeting",
                        promptArgs, getExamplePrompt);

        std::cout << "Server configured with:\n";
        std::cout << "  - Tools: echo, add\n";
        std::cout << "  - Resources: example://hello, example://info\n";
        std::cout << "  - Prompts: greeting\n";
        std::cout << "========================================\n";
        std::cout << "Starting server...\n";
        std::cout << "Press Ctrl+C to stop\n";
        std::cout << "========================================\n\n";

        server.start();

        std::cout << "\nServer stopped.\n";

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
