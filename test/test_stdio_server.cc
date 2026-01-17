#include "galay-mcp/server/McpStdioServer.h"
#include <iostream>
#include <string>

using namespace galay::mcp;

int main() {
    McpStdioServer server;

    // 设置服务器信息
    server.setServerInfo("test-mcp-server", "1.0.0");

    // 添加一个简单的加法工具
    Json addSchema;
    addSchema["type"] = "object";
    addSchema["properties"] = {
        {"a", {{"type", "number"}, {"description", "First number"}}},
        {"b", {{"type", "number"}, {"description", "Second number"}}}
    };
    addSchema["required"] = {"a", "b"};

    server.addTool("add", "Add two numbers", addSchema,
        [](const Json& args) -> std::expected<Json, McpError> {
            try {
                int a = args["a"];
                int b = args["b"];
                Json result;
                result["result"] = a + b;
                return result;
            } catch (const std::exception& e) {
                return std::unexpected(McpError::toolExecutionFailed(e.what()));
            }
        }
    );

    // 添加一个字符串连接工具
    Json concatSchema;
    concatSchema["type"] = "object";
    concatSchema["properties"] = {
        {"str1", {{"type", "string"}, {"description", "First string"}}},
        {"str2", {{"type", "string"}, {"description", "Second string"}}}
    };
    concatSchema["required"] = {"str1", "str2"};

    server.addTool("concat", "Concatenate two strings", concatSchema,
        [](const Json& args) -> std::expected<Json, McpError> {
            try {
                std::string str1 = args["str1"];
                std::string str2 = args["str2"];
                Json result;
                result["result"] = str1 + str2;
                return result;
            } catch (const std::exception& e) {
                return std::unexpected(McpError::toolExecutionFailed(e.what()));
            }
        }
    );

    // 添加一个简单的资源
    server.addResource("file:///test.txt", "test.txt", "Test file", "text/plain",
        [](const std::string& uri) -> std::expected<std::string, McpError> {
            if (uri == "file:///test.txt") {
                return "This is a test file content.";
            }
            return std::unexpected(McpError::resourceNotFound(uri));
        }
    );

    // 添加一个提示
    std::vector<Json> promptArgs;
    Json arg1;
    arg1["name"] = "topic";
    arg1["description"] = "The topic to write about";
    arg1["required"] = true;
    promptArgs.push_back(arg1);

    server.addPrompt("write_essay", "Generate an essay prompt", promptArgs,
        [](const std::string& name, const Json& args) -> std::expected<Json, McpError> {
            try {
                std::string topic = args["topic"];
                Json result;
                result["description"] = "Essay prompt";
                result["messages"] = Json::array();
                Json message;
                message["role"] = "user";
                message["content"] = {
                    {"type", "text"},
                    {"text", "Write an essay about: " + topic}
                };
                result["messages"].push_back(message);
                return result;
            } catch (const std::exception& e) {
                return std::unexpected(McpError::internalError(e.what()));
            }
        }
    );

    // 运行服务器
    std::cerr << "MCP Server started. Waiting for requests..." << std::endl;
    server.run();
    std::cerr << "MCP Server stopped." << std::endl;

    return 0;
}
