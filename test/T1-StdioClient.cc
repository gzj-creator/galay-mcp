#include "galay-mcp/client/McpStdioClient.h"
#include <iostream>
#include <string>

using namespace galay::mcp;

void printError(const McpError& error) {
    std::cerr << "Error: " << error.toString() << std::endl;
}

int main() {
    McpStdioClient client;

    std::cerr << "=== MCP Client Test ===" << std::endl;

    // 1. 初始化连接
    std::cerr << "\n1. Initializing connection..." << std::endl;
    auto initResult = client.initialize("test-mcp-client", "1.0.0");
    if (!initResult) {
        printError(initResult.error());
        return 1;
    }
    std::cerr << "✓ Initialized successfully" << std::endl;
    std::cerr << "  Server: " << client.getServerInfo().name
              << " v" << client.getServerInfo().version << std::endl;

    // 2. 获取工具列表
    std::cerr << "\n2. Listing tools..." << std::endl;
    auto toolsResult = client.listTools();
    if (!toolsResult) {
        printError(toolsResult.error());
        return 1;
    }
    std::cerr << "✓ Found " << toolsResult.value().size() << " tools:" << std::endl;
    for (const auto& tool : toolsResult.value()) {
        std::cerr << "  - " << tool.name << ": " << tool.description << std::endl;
    }

    // 3. 调用加法工具
    std::cerr << "\n3. Calling 'add' tool..." << std::endl;
    Json addArgs;
    addArgs["a"] = 10;
    addArgs["b"] = 20;
    auto addResult = client.callTool("add", addArgs);
    if (!addResult) {
        printError(addResult.error());
        return 1;
    }
    std::cerr << "✓ Result: " << addResult.value().dump() << std::endl;

    // 4. 调用字符串连接工具
    std::cerr << "\n4. Calling 'concat' tool..." << std::endl;
    Json concatArgs;
    concatArgs["str1"] = "Hello, ";
    concatArgs["str2"] = "World!";
    auto concatResult = client.callTool("concat", concatArgs);
    if (!concatResult) {
        printError(concatResult.error());
        return 1;
    }
    std::cerr << "✓ Result: " << concatResult.value().dump() << std::endl;

    // 5. 获取资源列表
    std::cerr << "\n5. Listing resources..." << std::endl;
    auto resourcesResult = client.listResources();
    if (!resourcesResult) {
        printError(resourcesResult.error());
        return 1;
    }
    std::cerr << "✓ Found " << resourcesResult.value().size() << " resources:" << std::endl;
    for (const auto& resource : resourcesResult.value()) {
        std::cerr << "  - " << resource.uri << ": " << resource.name << std::endl;
    }

    // 6. 读取资源
    std::cerr << "\n6. Reading resource..." << std::endl;
    auto readResult = client.readResource("file:///test.txt");
    if (!readResult) {
        printError(readResult.error());
        return 1;
    }
    std::cerr << "✓ Content: " << readResult.value() << std::endl;

    // 7. 获取提示列表
    std::cerr << "\n7. Listing prompts..." << std::endl;
    auto promptsResult = client.listPrompts();
    if (!promptsResult) {
        printError(promptsResult.error());
        return 1;
    }
    std::cerr << "✓ Found " << promptsResult.value().size() << " prompts:" << std::endl;
    for (const auto& prompt : promptsResult.value()) {
        std::cerr << "  - " << prompt.name << ": " << prompt.description << std::endl;
    }

    // 8. 获取提示
    std::cerr << "\n8. Getting prompt..." << std::endl;
    Json promptArgs;
    promptArgs["topic"] = "Artificial Intelligence";
    auto promptResult = client.getPrompt("write_essay", promptArgs);
    if (!promptResult) {
        printError(promptResult.error());
        return 1;
    }
    std::cerr << "✓ Prompt: " << promptResult.value().dump(2) << std::endl;

    // 9. Ping测试
    std::cerr << "\n9. Sending ping..." << std::endl;
    auto pingResult = client.ping();
    if (!pingResult) {
        printError(pingResult.error());
        return 1;
    }
    std::cerr << "✓ Ping successful" << std::endl;

    // 10. 断开连接
    std::cerr << "\n10. Disconnecting..." << std::endl;
    client.disconnect();
    std::cerr << "✓ Disconnected" << std::endl;

    std::cerr << "\n=== All tests passed! ===" << std::endl;

    return 0;
}
