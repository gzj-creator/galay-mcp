#pragma once
// Auto prelude for transitional C++23 module builds on Clang/GCC/MSVC.
// Keep third-party/system/dependency headers in global module fragment.

#if __has_include(<atomic>)
#include <atomic>
#endif
#if __has_include(<charconv>)
#include <charconv>
#endif
#if __has_include(<cstdint>)
#include <cstdint>
#endif
#if __has_include(<cstdio>)
#include <cstdio>
#endif
#if __has_include(<expected>)
#include <expected>
#endif
#if __has_include(<functional>)
#include <functional>
#endif
#if __has_include(<iostream>)
#include <iostream>
#endif
#if __has_include(<map>)
#include <map>
#endif
#if __has_include(<memory>)
#include <memory>
#endif
#if __has_include(<mutex>)
#include <mutex>
#endif
#if __has_include(<optional>)
#include <optional>
#endif
#if __has_include(<shared_mutex>)
#include <shared_mutex>
#endif
#if __has_include(<simdjson.h>)
#include <simdjson.h>
#endif
#if __has_include(<sstream>)
#include <sstream>
#endif
#if __has_include(<stdexcept>)
#include <stdexcept>
#endif
#if __has_include(<string>)
#include <string>
#endif
#if __has_include(<string_view>)
#include <string_view>
#endif
#if __has_include(<system_error>)
#include <system_error>
#endif
#if __has_include(<unordered_map>)
#include <unordered_map>
#endif
#if __has_include(<utility>)
#include <utility>
#endif
#if __has_include(<vector>)
#include <vector>
#endif
#if __has_include("galay-http/kernel/http/HttpClient.h")
#include "galay-http/kernel/http/HttpClient.h"
#endif
#if __has_include("galay-http/kernel/http/HttpRouter.h")
#include "galay-http/kernel/http/HttpRouter.h"
#endif
#if __has_include("galay-http/kernel/http/HttpServer.h")
#include "galay-http/kernel/http/HttpServer.h"
#endif
#if __has_include("galay-http/utils/Http1_1ResponseBuilder.h")
#include "galay-http/utils/Http1_1ResponseBuilder.h"
#endif
#if __has_include("galay-kernel/kernel/Coroutine.h")
#include "galay-kernel/kernel/Coroutine.h"
#endif
#if __has_include("galay-kernel/kernel/Runtime.h")
#include "galay-kernel/kernel/Runtime.h"
#endif
#if __has_include("galay-mcp/client/McpHttpClient.h")
#include "galay-mcp/client/McpHttpClient.h"
#endif
#if __has_include("galay-mcp/client/McpStdioClient.h")
#include "galay-mcp/client/McpStdioClient.h"
#endif
#if __has_include("galay-mcp/common/McpBase.h")
#include "galay-mcp/common/McpBase.h"
#endif
#if __has_include("galay-mcp/common/McpError.h")
#include "galay-mcp/common/McpError.h"
#endif
#if __has_include("galay-mcp/common/McpJson.h")
#include "galay-mcp/common/McpJson.h"
#endif
#if __has_include("galay-mcp/common/McpJsonParser.h")
#include "galay-mcp/common/McpJsonParser.h"
#endif
#if __has_include("galay-mcp/common/McpProtocolUtils.h")
#include "galay-mcp/common/McpProtocolUtils.h"
#endif
#if __has_include("galay-mcp/common/McpSchemaBuilder.h")
#include "galay-mcp/common/McpSchemaBuilder.h"
#endif
#if __has_include("galay-mcp/module/ModulePrelude.hpp")
#include "galay-mcp/module/ModulePrelude.hpp"
#endif
#if __has_include("galay-mcp/server/McpHttpServer.h")
#include "galay-mcp/server/McpHttpServer.h"
#endif
#if __has_include("galay-mcp/server/McpStdioServer.h")
#include "galay-mcp/server/McpStdioServer.h"
#endif
