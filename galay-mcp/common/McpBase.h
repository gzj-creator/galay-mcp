#ifndef GALAY_MCP_COMMON_MCPBASE_H
#define GALAY_MCP_COMMON_MCPBASE_H

#include "galay-mcp/common/McpJson.h"
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace galay {
namespace mcp {

// MCP协议版本
constexpr const char* MCP_VERSION = "2024-11-05";
constexpr const char* JSONRPC_VERSION = "2.0";

// MCP消息类型
enum class MessageType {
    Request,
    Response,
    Notification,
    Error
};

// MCP方法名称
namespace Methods {
    constexpr const char* INITIALIZE = "initialize";
    constexpr const char* INITIALIZED = "notifications/initialized";
    constexpr const char* PING = "ping";
    constexpr const char* TOOLS_LIST = "tools/list";
    constexpr const char* TOOLS_CALL = "tools/call";
    constexpr const char* RESOURCES_LIST = "resources/list";
    constexpr const char* RESOURCES_READ = "resources/read";
    constexpr const char* PROMPTS_LIST = "prompts/list";
    constexpr const char* PROMPTS_GET = "prompts/get";
}

// 内容类型
enum class ContentType {
    Text,
    Image,
    Resource
};

// 内容项
struct Content {
    ContentType type{ContentType::Text};
    std::string text;           // 用于Text类型
    std::string data;           // 用于Image类型（base64编码）
    std::string mimeType;       // 用于Image类型
    std::string uri;            // 用于Resource类型

    JsonString toJson() const;
    static std::expected<Content, McpError> fromJson(const JsonElement& element);
};

// 工具定义
struct Tool {
    std::string name;
    std::string description;
    JsonString inputSchema;           // JSON Schema格式

    JsonString toJson() const;
    static std::expected<Tool, McpError> fromJson(const JsonElement& element);
};

// 资源定义
struct Resource {
    std::string uri;
    std::string name;
    std::string description;
    std::string mimeType;

    JsonString toJson() const;
    static std::expected<Resource, McpError> fromJson(const JsonElement& element);
};

// 提示参数定义
struct PromptArgument {
    std::string name;
    std::string description;
    bool required{false};

    JsonString toJson() const;
    static std::expected<PromptArgument, McpError> fromJson(const JsonElement& element);
};

// 提示定义
struct Prompt {
    std::string name;
    std::string description;
    std::vector<PromptArgument> arguments;

    JsonString toJson() const;
    static std::expected<Prompt, McpError> fromJson(const JsonElement& element);
};

// 客户端信息
struct ClientInfo {
    std::string name;
    std::string version;

    JsonString toJson() const;
    static std::expected<ClientInfo, McpError> fromJson(const JsonElement& element);
};

// 服务器信息
struct ServerInfo {
    std::string name;
    std::string version;
    JsonString capabilities;

    JsonString toJson() const;
    static std::expected<ServerInfo, McpError> fromJson(const JsonElement& element);
};

// 服务器能力
struct ServerCapabilities {
    bool tools = false;
    bool resources = false;
    bool prompts = false;
    bool logging = false;

    JsonString toJson() const;
    static std::expected<ServerCapabilities, McpError> fromJson(const JsonElement& element);
};

// 初始化请求参数
struct InitializeParams {
    std::string protocolVersion;
    ClientInfo clientInfo;
    JsonString capabilities;

    JsonString toJson() const;
    static std::expected<InitializeParams, McpError> fromJson(const JsonElement& element);
};

// 初始化响应结果
struct InitializeResult {
    std::string protocolVersion;
    ServerInfo serverInfo;
    ServerCapabilities capabilities;

    JsonString toJson() const;
    static std::expected<InitializeResult, McpError> fromJson(const JsonElement& element);
};

// 工具调用参数
struct ToolCallParams {
    std::string name;
    JsonString arguments;

    JsonString toJson() const;
    static std::expected<ToolCallParams, McpError> fromJson(const JsonElement& element);
};

// 工具调用结果
struct ToolCallResult {
    std::vector<Content> content;
    bool isError = false;

    JsonString toJson() const;
    static std::expected<ToolCallResult, McpError> fromJson(const JsonElement& element);
};

// JSON-RPC请求（用于生成请求）
struct JsonRpcRequest {
    std::string jsonrpc = JSONRPC_VERSION;
    std::optional<int64_t> id;
    std::string method;
    std::optional<JsonString> params;

    JsonString toJson() const;
};

// JSON-RPC响应
struct JsonRpcResponse {
    std::string jsonrpc = JSONRPC_VERSION;
    int64_t id = 0;
    std::optional<JsonString> result;
    std::optional<JsonString> error;

    JsonString toJson() const;
    static std::expected<JsonRpcResponse, McpError> fromJson(const JsonElement& element);
};

// JSON-RPC通知
struct JsonRpcNotification {
    std::string jsonrpc = JSONRPC_VERSION;
    std::string method;
    std::optional<JsonString> params;

    JsonString toJson() const;
};

// JSON-RPC错误
struct JsonRpcError {
    int code = 0;
    std::string message;
    std::optional<JsonString> data;

    JsonString toJson() const;
    static std::expected<JsonRpcError, McpError> fromJson(const JsonElement& element);
};

// 错误码
namespace ErrorCodes {
    constexpr int PARSE_ERROR = -32700;
    constexpr int INVALID_REQUEST = -32600;
    constexpr int METHOD_NOT_FOUND = -32601;
    constexpr int INVALID_PARAMS = -32602;
    constexpr int INTERNAL_ERROR = -32603;
    constexpr int SERVER_ERROR_START = -32099;
    constexpr int SERVER_ERROR_END = -32000;
}

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_COMMON_MCPBASE_H
