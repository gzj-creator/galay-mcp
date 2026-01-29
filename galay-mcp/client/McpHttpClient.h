#ifndef GALAY_MCP_CLIENT_MCPHTTPCLIENT_H
#define GALAY_MCP_CLIENT_MCPHTTPCLIENT_H

#include "../common/McpBase.h"
#include "../common/McpError.h"
#include "galay-http/kernel/http/HttpClient.h"
#include "galay-kernel/kernel/Runtime.h"
#include <atomic>
#include <mutex>
#include <string>
#include <map>
#include <memory>

namespace galay {
namespace mcp {

/**
 * @brief 基于HTTP的MCP客户端
 *
 * 该类实现了MCP协议的客户端，通过HTTP POST请求发送JSON-RPC消息。
 */
class McpHttpClient {
public:
    /**
     * @brief 构造函数
     * @param runtime Runtime实例引用
     */
    explicit McpHttpClient(kernel::Runtime& runtime);
    ~McpHttpClient();

    // 禁止拷贝和移动
    McpHttpClient(const McpHttpClient&) = delete;
    McpHttpClient& operator=(const McpHttpClient&) = delete;
    McpHttpClient(McpHttpClient&&) = delete;
    McpHttpClient& operator=(McpHttpClient&&) = delete;

    /**
     * @brief 连接到服务器
     * @param url 服务器URL (例如: http://localhost:8080/mcp)
     * @return 成功返回void，失败返回错误信息
     */
    std::expected<void, McpError> connect(const std::string& url);

    /**
     * @brief 初始化连接
     * @param clientName 客户端名称
     * @param clientVersion 客户端版本
     * @return 成功返回void，失败返回错误信息
     */
    std::expected<void, McpError> initialize(const std::string& clientName,
                                             const std::string& clientVersion);

    /**
     * @brief 调用工具
     * @param toolName 工具名称
     * @param arguments 工具参数
     * @return 成功返回工具执行结果，失败返回错误信息
     */
    std::expected<Json, McpError> callTool(const std::string& toolName,
                                           const Json& arguments);

    /**
     * @brief 获取工具列表
     * @return 成功返回工具列表，失败返回错误信息
     */
    std::expected<std::vector<Tool>, McpError> listTools();

    /**
     * @brief 获取资源列表
     * @return 成功返回资源列表，失败返回错误信息
     */
    std::expected<std::vector<Resource>, McpError> listResources();

    /**
     * @brief 读取资源
     * @param uri 资源URI
     * @return 成功返回资源内容，失败返回错误信息
     */
    std::expected<std::string, McpError> readResource(const std::string& uri);

    /**
     * @brief 获取提示列表
     * @return 成功返回提示列表，失败返回错误信息
     */
    std::expected<std::vector<Prompt>, McpError> listPrompts();

    /**
     * @brief 获取提示
     * @param name 提示名称
     * @param arguments 提示参数
     * @return 成功返回提示内容，失败返回错误信息
     */
    std::expected<Json, McpError> getPrompt(const std::string& name,
                                            const Json& arguments);

    /**
     * @brief 发送ping请求
     * @return 成功返回void，失败返回错误信息
     */
    std::expected<void, McpError> ping();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 检查是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const;

    /**
     * @brief 获取服务器信息
     */
    const ServerInfo& getServerInfo() const;

    /**
     * @brief 获取服务器能力
     */
    const ServerCapabilities& getServerCapabilities() const;

private:
    // 发送请求并等待响应
    std::expected<Json, McpError> sendRequest(const std::string& method,
                                              const Json& params);

    // 生成请求ID
    int64_t generateRequestId();

private:
    // Runtime引用
    kernel::Runtime& m_runtime;

    // HTTP客户端
    std::unique_ptr<http::HttpClient> m_httpClient;

    // 客户端信息
    std::string m_clientName;
    std::string m_clientVersion;

    // 服务器信息
    ServerInfo m_serverInfo;
    ServerCapabilities m_serverCapabilities;

    // 连接状态
    std::atomic<bool> m_connected;
    std::atomic<bool> m_initialized;

    // 请求ID计数器
    std::atomic<int64_t> m_requestIdCounter;

    // 互斥锁
    std::mutex m_requestMutex;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_CLIENT_MCPHTTPCLIENT_H
