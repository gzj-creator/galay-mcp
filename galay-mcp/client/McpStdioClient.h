#ifndef GALAY_MCP_CLIENT_MCPSTDIOCLIENT_H
#define GALAY_MCP_CLIENT_MCPSTDIOCLIENT_H

#include "galay-mcp/common/McpBase.h"
#include "galay-mcp/common/McpError.h"
#include "galay-mcp/common/McpJsonParser.h"
#include <atomic>
#include <mutex>
#include <iostream>
#include <map>

namespace galay {
namespace mcp {

/**
 * @brief 基于标准输入输出的MCP客户端
 *
 * 该类实现了MCP协议的客户端，通过stdout发送请求，通过stdin接收响应。
 * 每条消息以换行符分隔，使用JSON-RPC 2.0格式。
 */
class McpStdioClient {
public:
    McpStdioClient();
    ~McpStdioClient();

    // 禁止拷贝和移动
    McpStdioClient(const McpStdioClient&) = delete;
    McpStdioClient& operator=(const McpStdioClient&) = delete;
    McpStdioClient(McpStdioClient&&) = delete;
    McpStdioClient& operator=(McpStdioClient&&) = delete;

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
    std::expected<JsonString, McpError> callTool(const std::string& toolName,
                                                 const JsonString& arguments);

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
    std::expected<JsonString, McpError> getPrompt(const std::string& name,
                                                  const JsonString& arguments);

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
    std::expected<JsonString, McpError> sendRequest(const std::string& method,
                                                    const std::optional<JsonString>& params);

    // 发送通知（不等待响应）
    std::expected<void, McpError> sendNotification(const std::string& method,
                                                   const std::optional<JsonString>& params);

    // 读取一行JSON消息
    std::expected<std::string, McpError> readMessage();

    // 写入一行JSON消息
    std::expected<void, McpError> writeMessage(const JsonString& message);

    // 生成请求ID
    int64_t generateRequestId();

private:
    // 客户端信息
    std::string m_clientName;
    std::string m_clientVersion;

    // 服务器信息
    ServerInfo m_serverInfo;
    ServerCapabilities m_serverCapabilities;

    // 初始化状态
    std::atomic<bool> m_initialized;

    // 请求ID计数器
    std::atomic<int64_t> m_requestIdCounter;

    // 输入输出流
    std::istream* m_input;
    std::ostream* m_output;
    std::mutex m_outputMutex;
    std::mutex m_inputMutex;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_CLIENT_MCPSTDIOCLIENT_H
