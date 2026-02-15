#ifndef GALAY_MCP_SERVER_MCPHTTPSERVER_H
#define GALAY_MCP_SERVER_MCPHTTPSERVER_H

#include "galay-mcp/common/McpBase.h"
#include "galay-mcp/common/McpError.h"
#include "galay-mcp/common/McpJsonParser.h"
#include "galay-http/kernel/http/HttpServer.h"
#include "galay-http/kernel/http/HttpRouter.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>

namespace galay {
namespace mcp {

/**
 * @brief 基于HTTP的MCP服务器
 *
 * @note 非线程安全：addTool/addResource/addPrompt 必须在 start() 之前调用，
 *       服务器运行期间不支持动态添加工具、资源或提示。
 */
class McpHttpServer {
public:
    // 工具处理函数类型（协程）
    using ToolHandler = std::function<kernel::Coroutine(const JsonElement&, std::expected<JsonString, McpError>&)>;

    // 资源读取函数类型（协程）
    using ResourceReader = std::function<kernel::Coroutine(const std::string&, std::expected<std::string, McpError>&)>;

    // 提示获取函数类型（协程）
    using PromptGetter = std::function<kernel::Coroutine(const std::string&, const JsonElement&, std::expected<JsonString, McpError>&)>;

    McpHttpServer(const std::string& host = "0.0.0.0", int port = 8080);
    ~McpHttpServer();

    McpHttpServer(const McpHttpServer&) = delete;
    McpHttpServer& operator=(const McpHttpServer&) = delete;
    McpHttpServer(McpHttpServer&&) = delete;
    McpHttpServer& operator=(McpHttpServer&&) = delete;

    void setServerInfo(const std::string& name, const std::string& version);

    void addTool(const std::string& name,
                 const std::string& description,
                 const JsonString& inputSchema,
                 ToolHandler handler);

    void addResource(const std::string& uri,
                     const std::string& name,
                     const std::string& description,
                     const std::string& mimeType,
                     ResourceReader reader);

    void addPrompt(const std::string& name,
                   const std::string& description,
                   const std::vector<PromptArgument>& arguments,
                   PromptGetter getter);

    void start();
    void stop();
    bool isRunning() const;

private:
    // 发送JSON响应的协程（只有这一层是协程）
    kernel::Coroutine sendJsonResponse(http::HttpConn& conn, const JsonString& responseJson);

    // 处理JSON-RPC请求（协程）
    kernel::Coroutine processRequest(const std::string& requestBody, JsonString& responseJson, bool& connectionInitialized);

    // 处理各种方法（全部同步，除了需要调用handler的）
    JsonString handleInitialize(const JsonRpcRequestView& request, bool& connectionInitialized);
    JsonString handleToolsList(const JsonRpcRequestView& request, bool& connectionInitialized);
    kernel::Coroutine handleToolsCall(const JsonRpcRequestView& request, JsonString& responseJson, bool& connectionInitialized);
    JsonString handleResourcesList(const JsonRpcRequestView& request, bool& connectionInitialized);
    kernel::Coroutine handleResourcesRead(const JsonRpcRequestView& request, JsonString& responseJson, bool& connectionInitialized);
    JsonString handlePromptsList(const JsonRpcRequestView& request, bool& connectionInitialized);
    kernel::Coroutine handlePromptsGet(const JsonRpcRequestView& request, JsonString& responseJson, bool& connectionInitialized);
    JsonString handlePing(const JsonRpcRequestView& request);

    JsonString createErrorResponse(int64_t id, int code, const std::string& message, const std::string& details = "");

    const JsonString& getToolsListResult();
    const JsonString& getResourcesListResult();
    const JsonString& getPromptsListResult();

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

    struct ResourceInfo {
        Resource resource;
        ResourceReader reader;
    };
    std::unordered_map<std::string, ResourceInfo> m_resources;

    struct PromptInfo {
        Prompt prompt;
        PromptGetter getter;
    };
    std::unordered_map<std::string, PromptInfo> m_prompts;

    JsonString m_toolsListCache;
    JsonString m_resourcesListCache;
    JsonString m_promptsListCache;
    bool m_toolsCacheDirty;
    bool m_resourcesCacheDirty;
    bool m_promptsCacheDirty;

    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;

    std::unique_ptr<http::HttpServer> m_httpServer;
    std::unique_ptr<http::HttpRouter> m_router;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_SERVER_MCPHTTPSERVER_H
