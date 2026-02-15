module;

#include "galay-mcp/module/ModulePrelude.hpp"

export module galay.mcp;

export {
#include "galay-mcp/common/McpError.h"
#include "galay-mcp/common/McpJson.h"
#include "galay-mcp/common/McpBase.h"
#include "galay-mcp/common/McpJsonParser.h"
#include "galay-mcp/common/McpSchemaBuilder.h"
#include "galay-mcp/common/McpProtocolUtils.h"

#include "galay-mcp/client/McpStdioClient.h"
#include "galay-mcp/client/McpHttpClient.h"

#include "galay-mcp/server/McpStdioServer.h"
#include "galay-mcp/server/McpHttpServer.h"
}
