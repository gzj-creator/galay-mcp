#ifndef GALAY_MCP_CLIENT_MCPHTTPCLIENT_H
#define GALAY_MCP_CLIENT_MCPHTTPCLIENT_H

#include "../common/McpBase.h"
#include "../common/McpError.h"
#include "galay-http/kernel/http/HttpClient.h"
#include "galay-kernel/kernel/Runtime.h"
#include "galay-kernel/kernel/Coroutine.h"
#include <atomic>
#include <string>
#include <memory>

namespace galay {
namespace mcp {

/**
 * @brief 基于HTTP的MCP客户端（异步接口）
 *
 * 该类实现了MCP协议的客户端，通过HTTP POST请求发送JSON-RPC消息。
 * 需要co_await的接口返回Coroutine，简单接口直接返回结果。
 */
class McpHttpClient {
public:
    explicit McpHttpClient(kernel::Runtime& runtime);
    ~McpHttpClient();

    // 禁止拷贝和移动
    McpHttpClient(const McpHttpClient&) = delete;
    McpHttpClient& operator=(const McpHttpClient&) = delete;
    McpHttpClient(McpHttpClient&&) = delete;
    McpHttpClient& operator=(McpHttpClient&&) = delete;

    /**
     * @brief 连接到服务器（返回等待体）
     */
    async::ConnectAwaitable connect(const std::string& url);

    /**
     * @brief 初始化连接（协程，内部需要co_await发送请求）
     */
    kernel::Coroutine initialize(std::string clientName,
                                  std::string clientVersion,
                                  std::expected<void, McpError>& result);

    /**
     * @brief 调用工具（协程）
     */
    kernel::Coroutine callTool(std::string toolName,
                                Json arguments,
                                std::expected<Json, McpError>& result);

    /**
     * @brief 获取工具列表（协程）
     */
    kernel::Coroutine listTools(std::expected<std::vector<Tool>, McpError>& result);

    /**
     * @brief 获取资源列表（协程）
     */
    kernel::Coroutine listResources(std::expected<std::vector<Resource>, McpError>& result);

    /**
     * @brief 读取资源（协程）
     */
    kernel::Coroutine readResource(std::string uri,
                                    std::expected<std::string, McpError>& result);

    /**
     * @brief 获取提示列表（协程）
     */
    kernel::Coroutine listPrompts(std::expected<std::vector<Prompt>, McpError>& result);

    /**
     * @brief 获取提示（协程）
     */
    kernel::Coroutine getPrompt(std::string name,
                                 Json arguments,
                                 std::expected<Json, McpError>& result);

    /**
     * @brief 发送ping请求（协程）
     */
    kernel::Coroutine ping(std::expected<void, McpError>& result);

    /**
     * @brief 断开连接（返回等待体）
     */
    async::CloseAwaitable disconnect();

    // 简单的同步接口
    bool isConnected() const { return m_connected.load(); }
    bool isInitialized() const { return m_initialized.load(); }
    const ServerInfo& getServerInfo() const { return m_serverInfo; }
    const ServerCapabilities& getServerCapabilities() const { return m_serverCapabilities; }

private:
    // 发送请求（协程）
    kernel::Coroutine sendRequest(std::string method,
                                   Json params,
                                   std::expected<Json, McpError>& result);

    int64_t generateRequestId();

private:
    kernel::Runtime& m_runtime;
    std::unique_ptr<http::HttpClient> m_httpClient;
    std::string m_serverUrl;
    std::string m_clientName;
    std::string m_clientVersion;
    ServerInfo m_serverInfo;
    ServerCapabilities m_serverCapabilities;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_initialized{false};
    std::atomic<int64_t> m_requestIdCounter{0};
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_CLIENT_MCPHTTPCLIENT_H
