#include "McpBase.h"

namespace galay {
namespace mcp {

// Content 实现
Json Content::toJson() const {
    Json j;
    switch (type) {
        case ContentType::Text:
            j["type"] = "text";
            j["text"] = text;
            break;
        case ContentType::Image:
            j["type"] = "image";
            j["data"] = data;
            j["mimeType"] = mimeType;
            break;
        case ContentType::Resource:
            j["type"] = "resource";
            j["uri"] = uri;
            break;
    }
    return j;
}

Content Content::fromJson(const Json& j) {
    Content c;
    std::string typeStr = j["type"];
    if (typeStr == "text") {
        c.type = ContentType::Text;
        c.text = j["text"];
    } else if (typeStr == "image") {
        c.type = ContentType::Image;
        c.data = j["data"];
        c.mimeType = j["mimeType"];
    } else if (typeStr == "resource") {
        c.type = ContentType::Resource;
        c.uri = j["uri"];
    }
    return c;
}

// Tool 实现
Json Tool::toJson() const {
    Json j;
    j["name"] = name;
    j["description"] = description;
    j["inputSchema"] = inputSchema;
    return j;
}

Tool Tool::fromJson(const Json& j) {
    Tool t;
    t.name = j["name"];
    t.description = j["description"];
    t.inputSchema = j["inputSchema"];
    return t;
}

// Resource 实现
Json Resource::toJson() const {
    Json j;
    j["uri"] = uri;
    j["name"] = name;
    j["description"] = description;
    j["mimeType"] = mimeType;
    return j;
}

Resource Resource::fromJson(const Json& j) {
    Resource r;
    r.uri = j["uri"];
    r.name = j["name"];
    r.description = j["description"];
    r.mimeType = j["mimeType"];
    return r;
}

// Prompt 实现
Json Prompt::toJson() const {
    Json j;
    j["name"] = name;
    j["description"] = description;
    j["arguments"] = arguments;
    return j;
}

Prompt Prompt::fromJson(const Json& j) {
    Prompt p;
    p.name = j["name"];
    p.description = j["description"];
    p.arguments = j["arguments"];
    return p;
}

// ClientInfo 实现
Json ClientInfo::toJson() const {
    Json j;
    j["name"] = name;
    j["version"] = version;
    return j;
}

ClientInfo ClientInfo::fromJson(const Json& j) {
    ClientInfo c;
    c.name = j["name"];
    c.version = j["version"];
    return c;
}

// ServerInfo 实现
Json ServerInfo::toJson() const {
    Json j;
    j["name"] = name;
    j["version"] = version;
    j["capabilities"] = capabilities;
    return j;
}

ServerInfo ServerInfo::fromJson(const Json& j) {
    ServerInfo s;
    s.name = j["name"];
    s.version = j["version"];
    s.capabilities = j["capabilities"];
    return s;
}

// ServerCapabilities 实现
Json ServerCapabilities::toJson() const {
    Json j;
    if (tools) j["tools"] = Json::object();
    if (resources) j["resources"] = Json::object();
    if (prompts) j["prompts"] = Json::object();
    if (logging) j["logging"] = Json::object();
    return j;
}

ServerCapabilities ServerCapabilities::fromJson(const Json& j) {
    ServerCapabilities c;
    c.tools = j.contains("tools");
    c.resources = j.contains("resources");
    c.prompts = j.contains("prompts");
    c.logging = j.contains("logging");
    return c;
}

// InitializeParams 实现
Json InitializeParams::toJson() const {
    Json j;
    j["protocolVersion"] = protocolVersion;
    j["clientInfo"] = clientInfo.toJson();
    j["capabilities"] = capabilities;
    return j;
}

InitializeParams InitializeParams::fromJson(const Json& j) {
    InitializeParams p;
    p.protocolVersion = j["protocolVersion"];
    p.clientInfo = ClientInfo::fromJson(j["clientInfo"]);
    p.capabilities = j["capabilities"];
    return p;
}

// InitializeResult 实现
Json InitializeResult::toJson() const {
    Json j;
    j["protocolVersion"] = protocolVersion;
    j["serverInfo"] = serverInfo.toJson();
    j["capabilities"] = capabilities.toJson();
    return j;
}

InitializeResult InitializeResult::fromJson(const Json& j) {
    InitializeResult r;
    r.protocolVersion = j["protocolVersion"];
    r.serverInfo = ServerInfo::fromJson(j["serverInfo"]);
    r.capabilities = ServerCapabilities::fromJson(j["capabilities"]);
    return r;
}

// ToolCallParams 实现
Json ToolCallParams::toJson() const {
    Json j;
    j["name"] = name;
    j["arguments"] = arguments;
    return j;
}

ToolCallParams ToolCallParams::fromJson(const Json& j) {
    ToolCallParams p;
    p.name = j["name"];
    p.arguments = j["arguments"];
    return p;
}

// ToolCallResult 实现
Json ToolCallResult::toJson() const {
    Json j;
    Json contentArray = Json::array();
    for (const auto& c : content) {
        contentArray.push_back(c.toJson());
    }
    j["content"] = contentArray;
    if (isError) {
        j["isError"] = true;
    }
    return j;
}

ToolCallResult ToolCallResult::fromJson(const Json& j) {
    ToolCallResult r;
    if (j.contains("content")) {
        for (const auto& item : j["content"]) {
            r.content.push_back(Content::fromJson(item));
        }
    }
    if (j.contains("isError")) {
        r.isError = j["isError"];
    }
    return r;
}

// JsonRpcRequest 实现
Json JsonRpcRequest::toJson() const {
    Json j;
    j["jsonrpc"] = jsonrpc;
    if (id.has_value()) {
        j["id"] = id.value();
    }
    j["method"] = method;
    if (!params.is_null()) {
        j["params"] = params;
    }
    return j;
}

JsonRpcRequest JsonRpcRequest::fromJson(const Json& j) {
    JsonRpcRequest r;
    r.jsonrpc = j["jsonrpc"];
    if (j.contains("id")) {
        r.id = j["id"].get<int64_t>();
    }
    r.method = j["method"];
    if (j.contains("params")) {
        r.params = j["params"];
    }
    return r;
}

// JsonRpcResponse 实现
Json JsonRpcResponse::toJson() const {
    Json j;
    j["jsonrpc"] = jsonrpc;
    j["id"] = id;
    if (result.has_value()) {
        j["result"] = result.value();
    }
    if (error.has_value()) {
        j["error"] = error.value();
    }
    return j;
}

JsonRpcResponse JsonRpcResponse::fromJson(const Json& j) {
    JsonRpcResponse r;
    r.jsonrpc = j["jsonrpc"];
    r.id = j["id"].get<int64_t>();
    if (j.contains("result")) {
        r.result = j["result"];
    }
    if (j.contains("error")) {
        r.error = j["error"];
    }
    return r;
}

// JsonRpcNotification 实现
Json JsonRpcNotification::toJson() const {
    Json j;
    j["jsonrpc"] = jsonrpc;
    j["method"] = method;
    if (!params.is_null()) {
        j["params"] = params;
    }
    return j;
}

JsonRpcNotification JsonRpcNotification::fromJson(const Json& j) {
    JsonRpcNotification n;
    n.jsonrpc = j["jsonrpc"];
    n.method = j["method"];
    if (j.contains("params")) {
        n.params = j["params"];
    }
    return n;
}

// JsonRpcError 实现
Json JsonRpcError::toJson() const {
    Json j;
    j["code"] = code;
    j["message"] = message;
    if (data.has_value()) {
        j["data"] = data.value();
    }
    return j;
}

JsonRpcError JsonRpcError::fromJson(const Json& j) {
    JsonRpcError e;
    e.code = j["code"];
    e.message = j["message"];
    if (j.contains("data")) {
        e.data = j["data"];
    }
    return e;
}

} // namespace mcp
} // namespace galay
