#ifndef GALAY_MCP_COMMON_MCPPROTOCOLUTILS_H
#define GALAY_MCP_COMMON_MCPPROTOCOLUTILS_H

#include "galay-mcp/common/McpBase.h"
#include "galay-mcp/common/McpJson.h"
#include <string>
#include <vector>

namespace galay {
namespace mcp {
namespace protocol {

inline JsonString buildInitializeResult(const std::string& serverName,
                                        const std::string& serverVersion,
                                        bool hasTools,
                                        bool hasResources,
                                        bool hasPrompts) {
    InitializeResult result;
    result.protocolVersion = MCP_VERSION;
    result.serverInfo.name = serverName;
    result.serverInfo.version = serverVersion;
    result.serverInfo.capabilities = "{}";

    result.capabilities.tools = hasTools;
    result.capabilities.resources = hasResources;
    result.capabilities.prompts = hasPrompts;
    result.capabilities.logging = false;

    return result.toJson();
}

inline JsonRpcResponse makeResultResponse(int64_t id, const JsonString& result) {
    JsonRpcResponse response;
    response.id = id;
    response.result = result;
    return response;
}

inline JsonRpcResponse makeErrorResponse(int64_t id,
                                         int code,
                                         const std::string& message,
                                         const std::string& details = "") {
    JsonRpcError error;
    error.code = code;
    error.message = message;
    if (!details.empty()) {
        JsonWriter writer;
        writer.String(details);
        error.data = writer.TakeString();
    }

    JsonRpcResponse response;
    response.id = id;
    response.error = error.toJson();
    return response;
}

template <typename MapType, typename Extractor>
JsonString buildListResultFromMap(const MapType& map, const char* key, Extractor extractor) {
    JsonWriter writer;
    writer.StartObject();
    writer.Key(key);
    writer.StartArray();
    for (const auto& [name, info] : map) {
        writer.Raw(extractor(info).toJson());
    }
    writer.EndArray();
    writer.EndObject();
    return writer.TakeString();
}

} // namespace protocol
} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_COMMON_MCPPROTOCOLUTILS_H
