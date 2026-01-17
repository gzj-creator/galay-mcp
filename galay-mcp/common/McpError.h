#ifndef GALAY_MCP_COMMON_MCPERROR_H
#define GALAY_MCP_COMMON_MCPERROR_H

#include <string>
#include <system_error>

namespace galay {
namespace mcp {

// MCP错误类型
enum class McpErrorCode {
    Success = 0,

    // 连接相关错误
    ConnectionFailed = 1000,
    ConnectionClosed = 1001,
    ConnectionTimeout = 1002,

    // 协议相关错误
    ProtocolError = 2000,
    InvalidMessage = 2001,
    InvalidMethod = 2002,
    InvalidParams = 2003,

    // JSON-RPC错误
    ParseError = 3000,
    InvalidRequest = 3001,
    MethodNotFound = 3002,
    InternalError = 3003,

    // 工具相关错误
    ToolNotFound = 4000,
    ToolExecutionFailed = 4001,

    // 资源相关错误
    ResourceNotFound = 5000,
    ResourceAccessDenied = 5001,

    // 提示相关错误
    PromptNotFound = 6000,

    // 初始化错误
    InitializationFailed = 7000,
    AlreadyInitialized = 7001,
    NotInitialized = 7002,

    // IO错误
    ReadError = 8000,
    WriteError = 8001,

    // 其他错误
    Unknown = 9999
};

// MCP错误类
class McpError {
public:
    McpError() : m_code(McpErrorCode::Success), m_message("") {}

    McpError(McpErrorCode code, const std::string& message)
        : m_code(code), m_message(message) {}

    McpError(McpErrorCode code, const std::string& message, const std::string& details)
        : m_code(code), m_message(message), m_details(details) {}

    // 获取错误码
    McpErrorCode code() const { return m_code; }

    // 获取错误消息
    const std::string& message() const { return m_message; }

    // 获取错误详情
    const std::string& details() const { return m_details; }

    // 是否成功
    bool isSuccess() const { return m_code == McpErrorCode::Success; }

    // 转换为字符串
    std::string toString() const;

    // 转换为JSON-RPC错误码
    int toJsonRpcErrorCode() const;

    // 静态工厂方法
    static McpError success() {
        return McpError(McpErrorCode::Success, "");
    }

    static McpError connectionFailed(const std::string& details = "") {
        return McpError(McpErrorCode::ConnectionFailed, "Connection failed", details);
    }

    static McpError connectionClosed(const std::string& details = "") {
        return McpError(McpErrorCode::ConnectionClosed, "Connection closed", details);
    }

    static McpError protocolError(const std::string& details = "") {
        return McpError(McpErrorCode::ProtocolError, "Protocol error", details);
    }

    static McpError invalidMessage(const std::string& details = "") {
        return McpError(McpErrorCode::InvalidMessage, "Invalid message", details);
    }

    static McpError invalidMethod(const std::string& method) {
        return McpError(McpErrorCode::InvalidMethod, "Invalid method", method);
    }

    static McpError invalidParams(const std::string& details = "") {
        return McpError(McpErrorCode::InvalidParams, "Invalid parameters", details);
    }

    static McpError parseError(const std::string& details = "") {
        return McpError(McpErrorCode::ParseError, "Parse error", details);
    }

    static McpError invalidRequest(const std::string& details = "") {
        return McpError(McpErrorCode::InvalidRequest, "Invalid request", details);
    }

    static McpError methodNotFound(const std::string& method) {
        return McpError(McpErrorCode::MethodNotFound, "Method not found", method);
    }

    static McpError internalError(const std::string& details = "") {
        return McpError(McpErrorCode::InternalError, "Internal error", details);
    }

    static McpError toolNotFound(const std::string& toolName) {
        return McpError(McpErrorCode::ToolNotFound, "Tool not found", toolName);
    }

    static McpError toolExecutionFailed(const std::string& details = "") {
        return McpError(McpErrorCode::ToolExecutionFailed, "Tool execution failed", details);
    }

    static McpError resourceNotFound(const std::string& uri) {
        return McpError(McpErrorCode::ResourceNotFound, "Resource not found", uri);
    }

    static McpError promptNotFound(const std::string& name) {
        return McpError(McpErrorCode::PromptNotFound, "Prompt not found", name);
    }

    static McpError initializationFailed(const std::string& details = "") {
        return McpError(McpErrorCode::InitializationFailed, "Initialization failed", details);
    }

    static McpError alreadyInitialized() {
        return McpError(McpErrorCode::AlreadyInitialized, "Already initialized", "");
    }

    static McpError notInitialized() {
        return McpError(McpErrorCode::NotInitialized, "Not initialized", "");
    }

    static McpError readError(const std::string& details = "") {
        return McpError(McpErrorCode::ReadError, "Read error", details);
    }

    static McpError writeError(const std::string& details = "") {
        return McpError(McpErrorCode::WriteError, "Write error", details);
    }

    static McpError unknown(const std::string& details = "") {
        return McpError(McpErrorCode::Unknown, "Unknown error", details);
    }

private:
    McpErrorCode m_code;
    std::string m_message;
    std::string m_details;
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_COMMON_MCPERROR_H
