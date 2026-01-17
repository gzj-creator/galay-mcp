#ifndef GALAY_MCP_COMMON_MCPBASE_H
#define GALAY_MCP_COMMON_MCPBASE_H

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <expected>
#include <nlohmann/json.hpp>

namespace galay {
namespace mcp {

// JSON类型别名
using Json = nlohmann::json;

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
    ContentType type;
    std::string text;           // 用于Text类型
    std::string data;           // 用于Image类型（base64编码）
    std::string mimeType;       // 用于Image类型
    std::string uri;            // 用于Resource类型

    Json toJson() const;
    static Content fromJson(const Json& j);
};

// 工具定义
struct Tool {
    std::string name;
    std::string description;
    Json inputSchema;           // JSON Schema格式

    Json toJson() const;
    static Tool fromJson(const Json& j);
};

// 资源定义
struct Resource {
    std::string uri;
    std::string name;
    std::string description;
    std::string mimeType;

    Json toJson() const;
    static Resource fromJson(const Json& j);
};

// 提示定义
struct Prompt {
    std::string name;
    std::string description;
    std::vector<Json> arguments;

    Json toJson() const;
    static Prompt fromJson(const Json& j);
};

// 客户端信息
struct ClientInfo {
    std::string name;
    std::string version;

    Json toJson() const;
    static ClientInfo fromJson(const Json& j);
};

// 服务器信息
struct ServerInfo {
    std::string name;
    std::string version;
    Json capabilities;

    Json toJson() const;
    static ServerInfo fromJson(const Json& j);
};

// 服务器能力
struct ServerCapabilities {
    bool tools = false;
    bool resources = false;
    bool prompts = false;
    bool logging = false;

    Json toJson() const;
    static ServerCapabilities fromJson(const Json& j);
};

// 初始化请求参数
struct InitializeParams {
    std::string protocolVersion;
    ClientInfo clientInfo;
    Json capabilities;

    Json toJson() const;
    static InitializeParams fromJson(const Json& j);
};

// 初始化响应结果
struct InitializeResult {
    std::string protocolVersion;
    ServerInfo serverInfo;
    ServerCapabilities capabilities;

    Json toJson() const;
    static InitializeResult fromJson(const Json& j);
};

// 工具调用参数
struct ToolCallParams {
    std::string name;
    Json arguments;

    Json toJson() const;
    static ToolCallParams fromJson(const Json& j);
};

// 工具调用结果
struct ToolCallResult {
    std::vector<Content> content;
    bool isError = false;

    Json toJson() const;
    static ToolCallResult fromJson(const Json& j);
};

// JSON-RPC请求
struct JsonRpcRequest {
    std::string jsonrpc = JSONRPC_VERSION;
    std::optional<int64_t> id;
    std::string method;
    Json params;

    Json toJson() const;
    static JsonRpcRequest fromJson(const Json& j);
};

// JSON-RPC响应
struct JsonRpcResponse {
    std::string jsonrpc = JSONRPC_VERSION;
    int64_t id;
    std::optional<Json> result;
    std::optional<Json> error;

    Json toJson() const;
    static JsonRpcResponse fromJson(const Json& j);
};

// JSON-RPC通知
struct JsonRpcNotification {
    std::string jsonrpc = JSONRPC_VERSION;
    std::string method;
    Json params;

    Json toJson() const;
    static JsonRpcNotification fromJson(const Json& j);
};

// JSON-RPC错误
struct JsonRpcError {
    int code;
    std::string message;
    std::optional<Json> data;

    Json toJson() const;
    static JsonRpcError fromJson(const Json& j);
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
