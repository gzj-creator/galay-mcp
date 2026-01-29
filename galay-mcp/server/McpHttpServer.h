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
 *
 * 该类实现了MCP协议的服务器端，通过HTTP接收请求并发送响应。
 * 使用JSON-RPC 2.0格式进行通信。
 */
class McpHttpServer {
public:
    /**
     * @brief 构造函数
     * @param host 监听地址
     * @param port 监听端口
     */
    McpHttpServer(const std::string& host = "0.0.0.0", int port = 8080);
    ~McpHttpServer();

    // 禁止拷贝和移动
    McpHttpServer(const McpHttpServer&) = delete;
    McpHttpServer& operator=(const McpHttpServer&) = delete;
    McpHttpServer(McpHttpServer&&) = delete;
    McpHttpServer& operator=(McpHttpServer&&) = delete;

    /**
     * @brief 设置服务器信息
     * @param name 服务器名称
     * @param version 服务器版本
     */
    void setServerInfo(const std::string& name, const std::string& version);

    /**
     * @brief 添加工具
     * @param name 工具名称
     * @param description 工具描述
     * @param inputSchema 输入参数的JSON Schema
     * @param handler 工具处理函数
     */
    void addTool(const std::string& name,
                 const std::string& description,
                 const Json& inputSchema,
                 ToolHandler handler);

    /**
     * @brief 添加资源
     * @param uri 资源URI
     * @param name 资源名称
     * @param description 资源描述
     * @param mimeType MIME类型
     * @param reader 资源读取函数
     */
    void addResource(const std::string& uri,
                     const std::string& name,
                     const std::string& description,
                     const std::string& mimeType,
                     ResourceReader reader);

    /**
     * @brief 添加提示
     * @param name 提示名称
     * @param description 提示描述
     * @param arguments 参数定义
     * @param getter 提示获取函数
     */
    void addPrompt(const std::string& name,
                   const std::string& description,
                   const std::vector<Json>& arguments,
                   PromptGetter getter);

    /**
     * @brief 启动服务器（阻塞）
     *
     * 该方法会阻塞当前线程，监听HTTP请求并处理，直到调用stop()。
     */
    void start();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 检查服务器是否正在运行
     */
    bool isRunning() const;

private:
    // 处理JSON-RPC请求（协程）
    kernel::Coroutine processRequest(const Json& requestJson, Json& responseJson);

    // 处理各种方法
    Json handleInitialize(const JsonRpcRequest& request);
    Json handleToolsList(const JsonRpcRequest& request);
    kernel::Coroutine handleToolsCall(const JsonRpcRequest& request, Json& responseJson);
    Json handleResourcesList(const JsonRpcRequest& request);
    kernel::Coroutine handleResourcesRead(const JsonRpcRequest& request, Json& responseJson);
    Json handlePromptsList(const JsonRpcRequest& request);
    kernel::Coroutine handlePromptsGet(const JsonRpcRequest& request, Json& responseJson);
    Json handlePing(const JsonRpcRequest& request);

    // 创建错误响应
    Json createErrorResponse(int64_t id, int code, const std::string& message, const std::string& details = "");

private:
    // 服务器配置
    std::string m_host;
    int m_port;

    // 服务器信息
    std::string m_serverName;
    std::string m_serverVersion;

    // 工具注册表
    struct ToolInfo {
        Tool tool;
        ToolHandler handler;
    };
    std::unordered_map<std::string, ToolInfo> m_tools;
    mutable std::shared_mutex m_toolsMutex;

    // 资源注册表
    struct ResourceInfo {
        Resource resource;
        ResourceReader reader;
    };
    std::unordered_map<std::string, ResourceInfo> m_resources;
    mutable std::shared_mutex m_resourcesMutex;

    // 提示注册表
    struct PromptInfo {
        Prompt prompt;
        PromptGetter getter;
    };
    std::unordered_map<std::string, PromptInfo> m_prompts;
    mutable std::shared_mutex m_promptsMutex;

    // 运行状态
    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;

    // HTTP服务器
    std::unique_ptr<http::HttpServer> m_httpServer;
    std::unique_ptr<http::HttpRouter> m_router;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_SERVER_MCPHTTPSERVER_H
