#include "galay-mcp/server/McpHttpServer.h"
#include "galay-http/utils/Http1_1ResponseBuilder.h"
#include "galay-mcp/common/McpProtocolUtils.h"

namespace galay {
namespace mcp {

namespace {

JsonString EmptyObjectString() {
    return "{}";
}

} // namespace

McpHttpServer::McpHttpServer(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
    , m_serverName("galay-mcp-http-server")
    , m_serverVersion("1.0.0")
    , m_toolsCacheDirty(true)
    , m_resourcesCacheDirty(true)
    , m_promptsCacheDirty(true)
    , m_running(false)
    , m_initialized(false) {
}

McpHttpServer::~McpHttpServer() {
    stop();
}

void McpHttpServer::setServerInfo(const std::string& name, const std::string& version) {
    m_serverName = name;
    m_serverVersion = version;
}

void McpHttpServer::addTool(const std::string& name,
                             const std::string& description,
                             const JsonString& inputSchema,
                             McpHttpServer::ToolHandler handler) {
    Tool tool;
    tool.name = name;
    tool.description = description;
    tool.inputSchema = inputSchema;

    ToolInfo info;
    info.tool = tool;
    info.handler = handler;

    m_tools[name] = info;
    m_toolsCacheDirty = true;
}

void McpHttpServer::addResource(const std::string& uri,
                                 const std::string& name,
                                 const std::string& description,
                                 const std::string& mimeType,
                                 McpHttpServer::ResourceReader reader) {
    Resource resource;
    resource.uri = uri;
    resource.name = name;
    resource.description = description;
    resource.mimeType = mimeType;

    ResourceInfo info;
    info.resource = resource;
    info.reader = reader;

    m_resources[uri] = info;
    m_resourcesCacheDirty = true;
}

void McpHttpServer::addPrompt(const std::string& name,
                               const std::string& description,
                               const std::vector<PromptArgument>& arguments,
                               McpHttpServer::PromptGetter getter) {
    Prompt prompt;
    prompt.name = name;
    prompt.description = description;
    prompt.arguments = arguments;

    PromptInfo info;
    info.prompt = prompt;
    info.getter = getter;

    m_prompts[name] = info;
    m_promptsCacheDirty = true;
}

void McpHttpServer::start() {
    if (m_running) {
        return;
    }

    m_router = std::make_unique<http::HttpRouter>();

    auto* serverPtr = this;
    m_router->addHandler<http::HttpMethod::POST>("/mcp",
        [serverPtr](http::HttpConn& conn, http::HttpRequest req) -> kernel::Coroutine {
            // 每个连接独立的初始化状态（若已有全局初始化则允许短连接复用）
            bool connectionInitialized = false;

            // 处理第一个请求
            {
                const std::string& requestBody = req.bodyStr();
                JsonString responseJson;
                try {
                    co_await serverPtr->processRequest(requestBody, responseJson, connectionInitialized).wait();
                } catch (const std::exception& e) {
                    responseJson = serverPtr->createErrorResponse(0, ErrorCodes::PARSE_ERROR,
                                                      "Parse error", e.what());
                }
                co_await serverPtr->sendJsonResponse(conn, responseJson).wait();
            }

            // Keep-Alive: 循环处理后续请求，直到连接关闭
            auto reader = conn.getReader();
            while (true) {
                http::HttpRequest nextReq;
                while (true) {
                    auto result = co_await reader.getRequest(nextReq);
                    if (!result) {
                        // 连接关闭或出错
                        co_await conn.close();
                        co_return;
                    }
                    if (result.value()) {
                        // 请求完整
                        break;
                    }
                    // 请求不完整，继续读取
                }

                const std::string& requestBody = nextReq.bodyStr();

                JsonString responseJson;
                try {
                    co_await serverPtr->processRequest(requestBody, responseJson, connectionInitialized).wait();
                } catch (const std::exception& e) {
                    responseJson = serverPtr->createErrorResponse(0, ErrorCodes::PARSE_ERROR,
                                                      "Parse error", e.what());
                }
                co_await serverPtr->sendJsonResponse(conn, responseJson).wait();
            }
        });

    http::HttpServerConfig config;
    config.host = m_host;
    config.port = static_cast<uint16_t>(m_port);
    config.backlog = 128;

    m_httpServer = std::make_unique<http::HttpServer>(config);
    m_running = true;
    m_httpServer->start(std::move(*m_router));

    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void McpHttpServer::stop() {
    m_running = false;
    m_initialized = false;
}

bool McpHttpServer::isRunning() const {
    return m_running;
}

kernel::Coroutine McpHttpServer::sendJsonResponse(http::HttpConn& conn, const JsonString& responseJson) {
    auto response = http::Http1_1ResponseBuilder::ok()
        .header("Server", m_serverName + "/" + m_serverVersion)
        .header("Content-Type", "application/json")
        .header("Connection", "keep-alive")
        .json(responseJson)
        .build();

    auto writer = conn.getWriter();
    while (true) {
        auto send_result = co_await writer.sendResponse(response);
        if (!send_result || send_result.value()) {
            break;
        }
    }
    co_return;
}

kernel::Coroutine McpHttpServer::processRequest(const std::string& requestBody, JsonString& responseJson, bool& connectionInitialized) {
    try {
        auto parsed = parseJsonRpcRequest(requestBody);
        if (!parsed) {
            responseJson = createErrorResponse(0,
                                   parsed.error().toJsonRpcErrorCode(),
                                   parsed.error().message(),
                                   parsed.error().details());
            co_return;
        }

        const JsonRpcRequestView& request = parsed.value().request;
        const std::string& method = request.method;

        if (method == Methods::INITIALIZE) {
            responseJson = handleInitialize(request, connectionInitialized);
        } else if (method == Methods::TOOLS_LIST) {
            responseJson = handleToolsList(request, connectionInitialized);
        } else if (method == Methods::TOOLS_CALL) {
            co_await handleToolsCall(request, responseJson, connectionInitialized).wait();
        } else if (method == Methods::RESOURCES_LIST) {
            responseJson = handleResourcesList(request, connectionInitialized);
        } else if (method == Methods::RESOURCES_READ) {
            co_await handleResourcesRead(request, responseJson, connectionInitialized).wait();
        } else if (method == Methods::PROMPTS_LIST) {
            responseJson = handlePromptsList(request, connectionInitialized);
        } else if (method == Methods::PROMPTS_GET) {
            co_await handlePromptsGet(request, responseJson, connectionInitialized).wait();
        } else if (method == Methods::PING) {
            responseJson = handlePing(request);
        } else {
            if (request.id.has_value()) {
                responseJson = createErrorResponse(request.id.value(),
                                         ErrorCodes::METHOD_NOT_FOUND,
                                         "Method not found", method);
            } else {
                responseJson = EmptyObjectString();
            }
        }
    } catch (const std::exception& e) {
        responseJson = createErrorResponse(0, ErrorCodes::INVALID_REQUEST,
                                  "Invalid request", e.what());
    }
    co_return;
}

JsonString McpHttpServer::handleInitialize(const JsonRpcRequestView& request, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        return EmptyObjectString();
    }

    if (connectionInitialized) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Already initialized", "");
    }

    if (!request.hasParams) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                  "Invalid parameters", "Missing params");
    }

    auto paramsExp = InitializeParams::fromJson(request.params);
    if (!paramsExp) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                  "Invalid parameters", paramsExp.error().message());
    }

    JsonString result = protocol::buildInitializeResult(
        m_serverName,
        m_serverVersion,
        !m_tools.empty(),
        !m_resources.empty(),
        !m_prompts.empty());

    JsonRpcResponse response = protocol::makeResultResponse(request.id.value(), result);

    connectionInitialized = true;
    m_initialized.store(true, std::memory_order_relaxed);

    return response.toJson();
}

JsonString McpHttpServer::handleToolsList(const JsonRpcRequestView& request, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        return EmptyObjectString();
    }

    if (!connectionInitialized && !m_initialized.load(std::memory_order_relaxed)) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
    }

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), getToolsListResult());

    return response.toJson();
}

kernel::Coroutine McpHttpServer::handleToolsCall(const JsonRpcRequestView& request, JsonString& responseJson, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        responseJson = EmptyObjectString();
        co_return;
    }

    if (!connectionInitialized && !m_initialized.load(std::memory_order_relaxed)) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
        co_return;
    }

    try {
        if (!request.hasParams) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Missing params");
            co_return;
        }

        JsonObject paramsObj;
        if (!JsonHelper::GetObject(request.params, paramsObj)) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Params must be object");
            co_return;
        }

        std::string toolName;
        if (!JsonHelper::GetString(paramsObj, "name", toolName)) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Missing tool name");
            co_return;
        }

        auto it = m_tools.find(toolName);
        if (it == m_tools.end()) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                                      "Tool not found", toolName);
            co_return;
        }

        McpHttpServer::ToolHandler handler = it->second.handler;

        JsonElement arguments = JsonHelper::EmptyObject();
        JsonElement argsElement;
        if (JsonHelper::GetElement(paramsObj, "arguments", argsElement)) {
            arguments = argsElement;
        }

        // 调用工具处理函数（协程）
        std::expected<JsonString, McpError> result;
        co_await handler(arguments, result).wait();

        if (!result) {
            responseJson = createErrorResponse(request.id.value(),
                                      result.error().toJsonRpcErrorCode(),
                                      result.error().message(),
                                      result.error().details());
            co_return;
        }

        ToolCallResult callResult;
        Content content;
        content.type = ContentType::Text;
        content.text = result.value();
        callResult.content.push_back(content);

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = callResult.toJson();

        responseJson = response.toJson();

    } catch (const std::exception& e) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                                  "Internal error", e.what());
    }
    co_return;
}

JsonString McpHttpServer::handleResourcesList(const JsonRpcRequestView& request, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        return EmptyObjectString();
    }

    if (!connectionInitialized && !m_initialized.load(std::memory_order_relaxed)) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
    }

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), getResourcesListResult());

    return response.toJson();
}

kernel::Coroutine McpHttpServer::handleResourcesRead(const JsonRpcRequestView& request, JsonString& responseJson, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        responseJson = EmptyObjectString();
        co_return;
    }

    if (!connectionInitialized && !m_initialized.load(std::memory_order_relaxed)) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
        co_return;
    }

    try {
        if (!request.hasParams) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Missing params");
            co_return;
        }

        JsonObject paramsObj;
        if (!JsonHelper::GetObject(request.params, paramsObj)) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Params must be object");
            co_return;
        }

        std::string uri;
        if (!JsonHelper::GetString(paramsObj, "uri", uri)) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Missing uri");
            co_return;
        }

        auto it = m_resources.find(uri);
        if (it == m_resources.end()) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                                      "Resource not found", uri);
            co_return;
        }

        McpHttpServer::ResourceReader reader = it->second.reader;

        // 调用资源读取函数（协程）
        std::expected<std::string, McpError> result;
        co_await reader(uri, result).wait();

        if (!result) {
            responseJson = createErrorResponse(request.id.value(),
                                      result.error().toJsonRpcErrorCode(),
                                      result.error().message(),
                                      result.error().details());
            co_return;
        }

        Content content;
        content.type = ContentType::Text;
        content.text = result.value();

        JsonWriter resultWriter;
        resultWriter.StartObject();
        resultWriter.Key("contents");
        resultWriter.StartArray();
        resultWriter.Raw(content.toJson());
        resultWriter.EndArray();
        resultWriter.EndObject();

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = resultWriter.TakeString();

        responseJson = response.toJson();

    } catch (const std::exception& e) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                                  "Internal error", e.what());
    }
    co_return;
}

JsonString McpHttpServer::handlePromptsList(const JsonRpcRequestView& request, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        return EmptyObjectString();
    }

    if (!connectionInitialized && !m_initialized.load(std::memory_order_relaxed)) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
    }

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), getPromptsListResult());

    return response.toJson();
}

kernel::Coroutine McpHttpServer::handlePromptsGet(const JsonRpcRequestView& request, JsonString& responseJson, bool& connectionInitialized) {
    if (!request.id.has_value()) {
        responseJson = EmptyObjectString();
        co_return;
    }

    if (!connectionInitialized && !m_initialized.load(std::memory_order_relaxed)) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
        co_return;
    }

    try {
        if (!request.hasParams) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Missing params");
            co_return;
        }

        JsonObject paramsObj;
        if (!JsonHelper::GetObject(request.params, paramsObj)) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Params must be object");
            co_return;
        }

        std::string name;
        if (!JsonHelper::GetString(paramsObj, "name", name)) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                      "Invalid parameters", "Missing prompt name");
            co_return;
        }

        JsonElement arguments = JsonHelper::EmptyObject();
        JsonElement argsElement;
        if (JsonHelper::GetElement(paramsObj, "arguments", argsElement)) {
            arguments = argsElement;
        }

        auto it = m_prompts.find(name);
        if (it == m_prompts.end()) {
            responseJson = createErrorResponse(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                                      "Prompt not found", name);
            co_return;
        }

        McpHttpServer::PromptGetter getter = it->second.getter;

        // 调用提示获取函数（协程）
        std::expected<JsonString, McpError> result;
        co_await getter(name, arguments, result).wait();

        if (!result) {
            responseJson = createErrorResponse(request.id.value(),
                                      result.error().toJsonRpcErrorCode(),
                                      result.error().message(),
                                      result.error().details());
            co_return;
        }

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = result.value();

        responseJson = response.toJson();

    } catch (const std::exception& e) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                                  "Internal error", e.what());
    }
    co_return;
}

JsonString McpHttpServer::handlePing(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return EmptyObjectString();
    }

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), EmptyObjectString());

    return response.toJson();
}

JsonString McpHttpServer::createErrorResponse(int64_t id, int code,
                                        const std::string& message,
                                        const std::string& details) {
    return protocol::makeErrorResponse(id, code, message, details).toJson();
}

const JsonString& McpHttpServer::getToolsListResult() {
    if (m_toolsCacheDirty) {
        m_toolsListCache = protocol::buildListResultFromMap(
            m_tools, "tools",
            [](const ToolInfo& info) -> const Tool& { return info.tool; });
        m_toolsCacheDirty = false;
    }
    return m_toolsListCache;
}

const JsonString& McpHttpServer::getResourcesListResult() {
    if (m_resourcesCacheDirty) {
        m_resourcesListCache = protocol::buildListResultFromMap(
            m_resources, "resources",
            [](const ResourceInfo& info) -> const Resource& { return info.resource; });
        m_resourcesCacheDirty = false;
    }
    return m_resourcesListCache;
}

const JsonString& McpHttpServer::getPromptsListResult() {
    if (m_promptsCacheDirty) {
        m_promptsListCache = protocol::buildListResultFromMap(
            m_prompts, "prompts",
            [](const PromptInfo& info) -> const Prompt& { return info.prompt; });
        m_promptsCacheDirty = false;
    }
    return m_promptsListCache;
}

} // namespace mcp
} // namespace galay
