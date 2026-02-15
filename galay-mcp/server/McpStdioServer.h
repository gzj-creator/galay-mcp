#ifndef GALAY_MCP_SERVER_MCPSTDIOSERVER_H
#define GALAY_MCP_SERVER_MCPSTDIOSERVER_H

#include "galay-mcp/common/McpBase.h"
#include "galay-mcp/common/McpError.h"
#include "galay-mcp/common/McpJsonParser.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <iostream>

namespace galay {
namespace mcp {

/**
 * @brief 基于标准输入输出的MCP服务器
 *
 * 该类实现了MCP协议的服务器端，通过stdin接收请求，通过stdout发送响应。
 * 每条消息以换行符分隔，使用JSON-RPC 2.0格式。
 */
class McpStdioServer {
public:
    // 工具处理函数类型
    using ToolHandler = std::function<std::expected<JsonString, McpError>(const JsonElement&)>;

    // 资源读取函数类型
    using ResourceReader = std::function<std::expected<std::string, McpError>(const std::string&)>;

    // 提示获取函数类型
    using PromptGetter = std::function<std::expected<JsonString, McpError>(const std::string&, const JsonElement&)>;

    McpStdioServer();
    ~McpStdioServer();

    // 禁止拷贝和移动
    McpStdioServer(const McpStdioServer&) = delete;
    McpStdioServer& operator=(const McpStdioServer&) = delete;
    McpStdioServer(McpStdioServer&&) = delete;
    McpStdioServer& operator=(McpStdioServer&&) = delete;

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
                 const JsonString& inputSchema,
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
                   const std::vector<PromptArgument>& arguments,
                   PromptGetter getter);

    /**
     * @brief 运行服务器（阻塞）
     *
     * 该方法会阻塞当前线程，从stdin读取请求并处理，直到收到停止信号或stdin关闭。
     */
    void run();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 检查服务器是否正在运行
     */
    bool isRunning() const;

private:
    // 处理请求
    void handleRequest(const JsonRpcRequestView& request);

    // 处理各种方法
    void handleInitialize(const JsonRpcRequestView& request);
    void handleToolsList(const JsonRpcRequestView& request);
    void handleToolsCall(const JsonRpcRequestView& request);
    void handleResourcesList(const JsonRpcRequestView& request);
    void handleResourcesRead(const JsonRpcRequestView& request);
    void handlePromptsList(const JsonRpcRequestView& request);
    void handlePromptsGet(const JsonRpcRequestView& request);
    void handlePing(const JsonRpcRequestView& request);

    // 发送响应
    void sendResponse(const JsonRpcResponse& response);
    void sendError(int64_t id, int code, const std::string& message, const std::string& details = "");
    void sendNotification(const std::string& method, const JsonString& params);

    // 读取一行JSON消息
    std::expected<std::string, McpError> readMessage();

    // 写入一行JSON消息
    std::expected<void, McpError> writeMessage(const JsonString& message);

private:
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

    JsonString m_toolsListCache;
    JsonString m_resourcesListCache;
    JsonString m_promptsListCache;

    // 运行状态
    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;

    // 输入输出流
    std::istream* m_input;
    std::ostream* m_output;
    std::mutex m_outputMutex;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_SERVER_MCPSTDIOSERVER_H
