#ifndef GALAY_MCP_COMMON_MCPJSONPARSER_H
#define GALAY_MCP_COMMON_MCPJSONPARSER_H

#include "galay-mcp/common/mcp_base.h"
#include "galay-mcp/common/mcp_error.h"
#include "galay-mcp/common/mcp_json.h"
#include <expected>
#include <string>
#include <string_view>

namespace galay {
namespace mcp {

struct JsonRpcRequestView {
    std::optional<int64_t> id;
    std::string method;
    JsonElement params;
    bool hasParams = false;
};

struct ParsedJsonRpcRequest {
    JsonDocument document;
    JsonRpcRequestView request;
};

struct JsonRpcResponseView {
    int64_t id = 0;
    JsonElement result;
    JsonElement error;
    bool hasResult = false;
    bool hasError = false;
};

struct ParsedJsonRpcResponse {
    JsonDocument document;
    JsonRpcResponseView response;
};

// Parse JSON-RPC request from raw JSON text.
std::expected<ParsedJsonRpcRequest, McpError> parseJsonRpcRequest(std::string_view body);

// Parse JSON-RPC response from raw JSON text.
std::expected<ParsedJsonRpcResponse, McpError> parseJsonRpcResponse(std::string_view body);

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_COMMON_MCPJSONPARSER_H
