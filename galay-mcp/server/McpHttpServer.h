#ifndef GALAY_MCP_SERVER_MCPHTTPSERVER_H
#define GALAY_MCP_SERVER_MCPHTTPSERVER_H

#include "../common/McpBase.h"
#include "../common/McpError.h"
#include "galay-http/kernel/http/HttpServer.h"
#include "galay-http/kernel/http/HttpRouter.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <shared_mutex>

namespace galay {
namespace mcp {

// 工具处理函数类型（协程）
using ToolHandler = std::function<kernel::Coroutine(const Json&, std::expected<Json, McpError>&)>;

// 资源读取函数类型（协程）
using ResourceReader = std::function<kernel::Coroutine(const std::string&, std::expected<std::string, McpError>&)>;

// 提示获取函数类型（协程）
using PromptGetter = std::function<kernel::Coroutine(const std::string&, const Json&, std::expected<Json, McpError>&)>;

/**
 * @brief 基于HTTP的MCP服务器
 */
class McpHttpServer {
public:
    McpHttpServer(const std::string& host = "0.0.0.0", int port = 8080);
    ~McpHttpServer();

    McpHttpServer(const McpHttpServer&) = delete;
    McpHttpServer& operator=(const McpHttpServer&) = delete;
    McpHttpServer(McpHttpServer&&) = delete;
    McpHttpServer& operator=(McpHttpServer&&) = delete;

    void setServerInfo(const std::string& name, const std::string& version);

    void addTool(const std::string& name,
                 const std::string& description,
                 const Json& inputSchema,
                 ToolHandler handler);

    void addResource(const std::string& uri,
                     const std::string& name,
                     const std::string& description,
                     const std::string& mimeType,
                     ResourceReader reader);

    void addPrompt(const std::string& name,
                   const std::string& description,
                   const std::vector<Json>& arguments,
                   PromptGetter getter);

    void start();
    void stop();
    bool isRunning() const;

private:
    // 发送JSON响应的协程（只有这一层是协程）
    kernel::Coroutine sendJsonResponse(http::HttpConn& conn, const Json& responseJson);

    // 处理JSON-RPC请求（协程）
    kernel::Coroutine processRequest(const Json& requestJson, Json& responseJson, bool& connectionInitialized);

    // 处理各种方法（全部同步，除了需要调用handler的）
    Json handleInitialize(const JsonRpcRequest& request, bool& connectionInitialized);
    Json handleToolsList(const JsonRpcRequest& request, bool& connectionInitialized);
    kernel::Coroutine handleToolsCall(const JsonRpcRequest& request, Json& responseJson, bool& connectionInitialized);
    Json handleResourcesList(const JsonRpcRequest& request, bool& connectionInitialized);
    kernel::Coroutine handleResourcesRead(const JsonRpcRequest& request, Json& responseJson, bool& connectionInitialized);
    Json handlePromptsList(const JsonRpcRequest& request, bool& connectionInitialized);
    kernel::Coroutine handlePromptsGet(const JsonRpcRequest& request, Json& responseJson, bool& connectionInitialized);
    Json handlePing(const JsonRpcRequest& request);

    Json createErrorResponse(int64_t id, int code, const std::string& message, const std::string& details = "");

private:
    std::string m_host;
    int m_port;
    std::string m_serverName;
    std::string m_serverVersion;

    struct ToolInfo {
        Tool tool;
        ToolHandler handler;
    };
    std::unordered_map<std::string, ToolInfo> m_tools;
    mutable std::shared_mutex m_toolsMutex;

    struct ResourceInfo {
        Resource resource;
        ResourceReader reader;
    };
    std::unordered_map<std::string, ResourceInfo> m_resources;
    mutable std::shared_mutex m_resourcesMutex;

    struct PromptInfo {
        Prompt prompt;
        PromptGetter getter;
    };
    std::unordered_map<std::string, PromptInfo> m_prompts;
    mutable std::shared_mutex m_promptsMutex;

    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;

    std::unique_ptr<http::HttpServer> m_httpServer;
    std::unique_ptr<http::HttpRouter> m_router;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_SERVER_MCPHTTPSERVER_H
