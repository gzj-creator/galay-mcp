#include "McpHttpServer.h"
#include "galay-http/utils/Http1_1ResponseBuilder.h"
#include <iostream>

namespace galay {
namespace mcp {

McpHttpServer::McpHttpServer(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
    , m_serverName("galay-mcp-http-server")
    , m_serverVersion("1.0.0")
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
                             const Json& inputSchema,
                             ToolHandler handler) {
    std::unique_lock<std::shared_mutex> lock(m_toolsMutex);

    Tool tool;
    tool.name = name;
    tool.description = description;
    tool.inputSchema = inputSchema;

    ToolInfo info;
    info.tool = tool;
    info.handler = handler;

    m_tools[name] = info;
}

void McpHttpServer::addResource(const std::string& uri,
                                 const std::string& name,
                                 const std::string& description,
                                 const std::string& mimeType,
                                 ResourceReader reader) {
    std::unique_lock<std::shared_mutex> lock(m_resourcesMutex);

    Resource resource;
    resource.uri = uri;
    resource.name = name;
    resource.description = description;
    resource.mimeType = mimeType;

    ResourceInfo info;
    info.resource = resource;
    info.reader = reader;

    m_resources[uri] = info;
}

void McpHttpServer::addPrompt(const std::string& name,
                               const std::string& description,
                               const std::vector<Json>& arguments,
                               PromptGetter getter) {
    std::unique_lock<std::shared_mutex> lock(m_promptsMutex);

    Prompt prompt;
    prompt.name = name;
    prompt.description = description;
    prompt.arguments = arguments;

    PromptInfo info;
    info.prompt = prompt;
    info.getter = getter;

    m_prompts[name] = info;
}

void McpHttpServer::start() {
    if (m_running) {
        return;
    }

    // 创建路由器
    m_router = std::make_unique<http::HttpRouter>();

    // 注册MCP端点 - 使用独立的协程函数
    auto* serverPtr = this;
    m_router->addHandler<http::HttpMethod::POST>("/mcp",
        [serverPtr](http::HttpConn& conn, http::HttpRequest req) -> kernel::Coroutine {
            // 获取请求体
            std::string requestBody = req.getBodyStr();

            Json responseJson;

            try {
                // 解析JSON请求
                Json requestJson = Json::parse(requestBody);

                // 处理请求（协程）
                co_await serverPtr->processRequest(requestJson, responseJson).wait();

            } catch (const std::exception& e) {
                // 解析失败，返回错误
                responseJson = serverPtr->createErrorResponse(0, ErrorCodes::PARSE_ERROR,
                                                  "Parse error", e.what());
            }

            // 构建HTTP响应
            auto response = http::Http1_1ResponseBuilder::ok()
                .header("Server", serverPtr->m_serverName + "/" + serverPtr->m_serverVersion)
                .header("Content-Type", "application/json")
                .json(responseJson.dump())
                .build();

            // 发送响应
            auto writer = conn.getWriter();
            while (true) {
                auto send_result = co_await writer.sendResponse(response);
                if (!send_result) {
                    co_return;
                }
                if (send_result.value()) {
                    break;
                }
            }

            co_await conn.close();
            co_return;
        });

    // 配置HTTP服务器
    http::HttpServerConfig config;
    config.host = m_host;
    config.port = m_port;
    config.backlog = 128;

    // 创建HTTP服务器
    m_httpServer = std::make_unique<http::HttpServer>(config);

    m_running = true;

    // 启动服务器（非阻塞）
    m_httpServer->start(std::move(*m_router));

    // 保持服务器运行
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

kernel::Coroutine McpHttpServer::processRequest(const Json& requestJson, Json& responseJson) {
    try {
        JsonRpcRequest request = JsonRpcRequest::fromJson(requestJson);
        const std::string& method = request.method;

        if (method == Methods::INITIALIZE) {
            responseJson = handleInitialize(request);
        } else if (method == Methods::TOOLS_LIST) {
            responseJson = handleToolsList(request);
        } else if (method == Methods::TOOLS_CALL) {
            co_await handleToolsCall(request, responseJson).wait();
        } else if (method == Methods::RESOURCES_LIST) {
            responseJson = handleResourcesList(request);
        } else if (method == Methods::RESOURCES_READ) {
            co_await handleResourcesRead(request, responseJson).wait();
        } else if (method == Methods::PROMPTS_LIST) {
            responseJson = handlePromptsList(request);
        } else if (method == Methods::PROMPTS_GET) {
            co_await handlePromptsGet(request, responseJson).wait();
        } else if (method == Methods::PING) {
            responseJson = handlePing(request);
        } else {
            if (request.id.has_value()) {
                responseJson = createErrorResponse(request.id.value(),
                                         ErrorCodes::METHOD_NOT_FOUND,
                                         "Method not found", method);
            } else {
                responseJson = Json::object();
            }
        }
    } catch (const std::exception& e) {
        responseJson = createErrorResponse(0, ErrorCodes::INVALID_REQUEST,
                                  "Invalid request", e.what());
    }
    co_return;
}

Json McpHttpServer::handleInitialize(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return Json::object();
    }

    if (m_initialized) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Already initialized", "");
    }

    try {
        InitializeParams params = InitializeParams::fromJson(request.params);

        // 构建响应
        InitializeResult result;
        result.protocolVersion = MCP_VERSION;
        result.serverInfo.name = m_serverName;
        result.serverInfo.version = m_serverVersion;
        result.serverInfo.capabilities = Json::object();

        // 设置能力
        result.capabilities.tools = !m_tools.empty();
        result.capabilities.resources = !m_resources.empty();
        result.capabilities.prompts = !m_prompts.empty();
        result.capabilities.logging = false;

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = result.toJson();

        m_initialized = true;

        return response.toJson();

    } catch (const std::exception& e) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_PARAMS,
                                  "Invalid parameters", e.what());
    }
}

Json McpHttpServer::handleToolsList(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return Json::object();
    }

    if (!m_initialized) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
    }

    std::shared_lock<std::shared_mutex> lock(m_toolsMutex);

    Json toolsArray = Json::array();
    for (const auto& [name, info] : m_tools) {
        toolsArray.push_back(info.tool.toJson());
    }

    Json result;
    result["tools"] = toolsArray;

    JsonRpcResponse response;
    response.id = request.id.value();
    response.result = result;

    return response.toJson();
}

kernel::Coroutine McpHttpServer::handleToolsCall(const JsonRpcRequest& request, Json& responseJson) {
    if (!request.id.has_value()) {
        responseJson = Json::object();
        co_return;
    }

    if (!m_initialized) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
        co_return;
    }

    try {
        ToolCallParams params = ToolCallParams::fromJson(request.params);

        ToolHandler handler;
        {
            std::shared_lock<std::shared_mutex> lock(m_toolsMutex);

            auto it = m_tools.find(params.name);
            if (it == m_tools.end()) {
                responseJson = createErrorResponse(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                                          "Tool not found", params.name);
                co_return;
            }

            handler = it->second.handler;
        }

        // 调用工具处理函数（协程）
        std::expected<Json, McpError> result;
        co_await handler(params.arguments, result).wait();

        if (!result) {
            responseJson = createErrorResponse(request.id.value(),
                                      result.error().toJsonRpcErrorCode(),
                                      result.error().message(),
                                      result.error().details());
            co_return;
        }

        // 构建响应
        ToolCallResult callResult;
        Content content;
        content.type = ContentType::Text;
        content.text = result.value().dump();
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

Json McpHttpServer::handleResourcesList(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return Json::object();
    }

    if (!m_initialized) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
    }

    std::shared_lock<std::shared_mutex> lock(m_resourcesMutex);

    Json resourcesArray = Json::array();
    for (const auto& [uri, info] : m_resources) {
        resourcesArray.push_back(info.resource.toJson());
    }

    Json result;
    result["resources"] = resourcesArray;

    JsonRpcResponse response;
    response.id = request.id.value();
    response.result = result;

    return response.toJson();
}

kernel::Coroutine McpHttpServer::handleResourcesRead(const JsonRpcRequest& request, Json& responseJson) {
    if (!request.id.has_value()) {
        responseJson = Json::object();
        co_return;
    }

    if (!m_initialized) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
        co_return;
    }

    try {
        std::string uri = request.params["uri"];

        ResourceReader reader;
        {
            std::shared_lock<std::shared_mutex> lock(m_resourcesMutex);

            auto it = m_resources.find(uri);
            if (it == m_resources.end()) {
                responseJson = createErrorResponse(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                                          "Resource not found", uri);
                co_return;
            }

            reader = it->second.reader;
        }

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

        // 构建响应
        Json contents = Json::array();
        Content content;
        content.type = ContentType::Text;
        content.text = result.value();
        contents.push_back(content.toJson());

        Json responseResult;
        responseResult["contents"] = contents;

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = responseResult;

        responseJson = response.toJson();

    } catch (const std::exception& e) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                                  "Internal error", e.what());
    }
    co_return;
}

Json McpHttpServer::handlePromptsList(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return Json::object();
    }

    if (!m_initialized) {
        return createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
    }

    std::shared_lock<std::shared_mutex> lock(m_promptsMutex);

    Json promptsArray = Json::array();
    for (const auto& [name, info] : m_prompts) {
        promptsArray.push_back(info.prompt.toJson());
    }

    Json result;
    result["prompts"] = promptsArray;

    JsonRpcResponse response;
    response.id = request.id.value();
    response.result = result;

    return response.toJson();
}

galay::kernel::Coroutine McpHttpServer::handlePromptsGet(const JsonRpcRequest& request, Json& responseJson) {
    if (!request.id.has_value()) {
        responseJson = Json::object();
        co_return;
    }

    if (!m_initialized) {
        responseJson = createErrorResponse(request.id.value(), ErrorCodes::INVALID_REQUEST,
                                  "Not initialized", "");
        co_return;
    }

    try {
        std::string name = request.params["name"];
        Json arguments = request.params.contains("arguments") ?
                        request.params["arguments"] : Json::object();

        PromptGetter getter;
        {
            std::shared_lock<std::shared_mutex> lock(m_promptsMutex);

            auto it = m_prompts.find(name);
            if (it == m_prompts.end()) {
                responseJson = createErrorResponse(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                                          "Prompt not found", name);
                co_return;
            }

            getter = it->second.getter;
        }

        // 调用提示获取函数（协程）
        std::expected<Json, McpError> result;
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

Json McpHttpServer::handlePing(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return Json::object();
    }

    JsonRpcResponse response;
    response.id = request.id.value();
    response.result = Json::object();

    return response.toJson();
}

Json McpHttpServer::createErrorResponse(int64_t id, int code,
                                        const std::string& message,
                                        const std::string& details) {
    JsonRpcError error;
    error.code = code;
    error.message = message;
    if (!details.empty()) {
        error.data = details;
    }

    JsonRpcResponse response;
    response.id = id;
    response.error = error.toJson();

    return response.toJson();
}

} // namespace mcp
} // namespace galay
