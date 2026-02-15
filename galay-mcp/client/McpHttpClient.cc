#include "galay-mcp/client/McpHttpClient.h"
#include "galay-mcp/common/McpJsonParser.h"

namespace galay {
namespace mcp {

namespace {

JsonString EmptyObjectString() {
    return "{}";
}

} // namespace

McpHttpClient::McpHttpClient(kernel::Runtime& runtime)
    : m_runtime(runtime) {
    m_httpClient = std::make_unique<http::HttpClient>();
}

McpHttpClient::~McpHttpClient() {
}

async::ConnectAwaitable McpHttpClient::connect(const std::string& url) {
    m_serverUrl = url;
    return m_httpClient->connect(url);
}

kernel::Coroutine McpHttpClient::initialize(std::string clientName,
                                             std::string clientVersion,
                                             std::expected<void, McpError>& result) {
    m_clientName = std::move(clientName);
    m_clientVersion = std::move(clientVersion);

    // 构建初始化请求
    InitializeParams params;
    params.protocolVersion = MCP_VERSION;
    params.clientInfo.name = m_clientName;
    params.clientInfo.version = m_clientVersion;
    params.capabilities = EmptyObjectString();

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::INITIALIZE, params.toJson(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    auto docExp = JsonDocument::Parse(response.value());
    if (!docExp) {
        result = std::unexpected(McpError::initializationFailed(docExp.error().details()));
        co_return;
    }

    auto initExp = InitializeResult::fromJson(docExp.value().Root());
    if (!initExp) {
        result = std::unexpected(McpError::initializationFailed(initExp.error().message()));
        co_return;
    }

    m_serverInfo = initExp.value().serverInfo;
    m_serverCapabilities = initExp.value().capabilities;
    m_initialized = true;
    m_connected = true;
    result = {};

    co_return;
}

kernel::Coroutine McpHttpClient::callTool(std::string toolName,
                                           JsonString arguments,
                                           std::expected<JsonString, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    ToolCallParams params;
    params.name = std::move(toolName);
    params.arguments = arguments.empty() ? EmptyObjectString() : std::move(arguments);

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::TOOLS_CALL, params.toJson(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    auto docExp = JsonDocument::Parse(response.value());
    if (!docExp) {
        result = std::unexpected(McpError::parseError(docExp.error().details()));
        co_return;
    }

    auto callExp = ToolCallResult::fromJson(docExp.value().Root());
    if (!callExp) {
        result = std::unexpected(McpError::parseError(callExp.error().message()));
        co_return;
    }

    const auto& callResult = callExp.value();
    if (callResult.isError) {
        result = std::unexpected(McpError::toolExecutionFailed("Tool returned error"));
        co_return;
    }
    if (callResult.content.empty()) {
        result = EmptyObjectString();
        co_return;
    }
    if (callResult.content[0].type == ContentType::Text) {
        result = callResult.content[0].text;
    } else {
        result = EmptyObjectString();
    }

    co_return;
}

kernel::Coroutine McpHttpClient::listTools(std::expected<std::vector<Tool>, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::TOOLS_LIST, EmptyObjectString(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    auto docExp = JsonDocument::Parse(response.value());
    if (!docExp) {
        result = std::unexpected(McpError::parseError(docExp.error().details()));
        co_return;
    }

    std::vector<Tool> tools;
    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "tools", arr)) {
            for (auto item : arr) {
                auto toolExp = Tool::fromJson(item);
                if (!toolExp) {
                    result = std::unexpected(McpError::parseError(toolExp.error().message()));
                    co_return;
                }
                tools.push_back(std::move(toolExp.value()));
            }
        }
    }
    result = std::move(tools);
    co_return;
}

kernel::Coroutine McpHttpClient::listResources(std::expected<std::vector<Resource>, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::RESOURCES_LIST, EmptyObjectString(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    auto docExp = JsonDocument::Parse(response.value());
    if (!docExp) {
        result = std::unexpected(McpError::parseError(docExp.error().details()));
        co_return;
    }

    std::vector<Resource> resources;
    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "resources", arr)) {
            for (auto item : arr) {
                auto resExp = Resource::fromJson(item);
                if (!resExp) {
                    result = std::unexpected(McpError::parseError(resExp.error().message()));
                    co_return;
                }
                resources.push_back(std::move(resExp.value()));
            }
        }
    }
    result = std::move(resources);
    co_return;
}

kernel::Coroutine McpHttpClient::readResource(std::string uri,
                                               std::expected<std::string, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    JsonWriter paramsWriter;
    paramsWriter.StartObject();
    paramsWriter.Key("uri");
    paramsWriter.String(std::move(uri));
    paramsWriter.EndObject();

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::RESOURCES_READ, paramsWriter.TakeString(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    auto docExp = JsonDocument::Parse(response.value());
    if (!docExp) {
        result = std::unexpected(McpError::parseError(docExp.error().details()));
        co_return;
    }

    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "contents", arr)) {
            for (auto item : arr) {
                auto contentExp = Content::fromJson(item);
                if (!contentExp) {
                    result = std::unexpected(McpError::parseError(contentExp.error().message()));
                    co_return;
                }
                if (contentExp.value().type == ContentType::Text) {
                    result = contentExp.value().text;
                    co_return;
                }
            }
        }
    }

    result = "";
    co_return;
}

kernel::Coroutine McpHttpClient::listPrompts(std::expected<std::vector<Prompt>, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::PROMPTS_LIST, EmptyObjectString(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    auto docExp = JsonDocument::Parse(response.value());
    if (!docExp) {
        result = std::unexpected(McpError::parseError(docExp.error().details()));
        co_return;
    }

    std::vector<Prompt> prompts;
    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "prompts", arr)) {
            for (auto item : arr) {
                auto promptExp = Prompt::fromJson(item);
                if (!promptExp) {
                    result = std::unexpected(McpError::parseError(promptExp.error().message()));
                    co_return;
                }
                prompts.push_back(std::move(promptExp.value()));
            }
        }
    }
    result = std::move(prompts);
    co_return;
}

kernel::Coroutine McpHttpClient::getPrompt(std::string name,
                                            JsonString arguments,
                                            std::expected<JsonString, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    JsonWriter paramsWriter;
    paramsWriter.StartObject();
    paramsWriter.Key("name");
    paramsWriter.String(std::move(name));
    if (!arguments.empty()) {
        paramsWriter.Key("arguments");
        paramsWriter.Raw(arguments);
    }
    paramsWriter.EndObject();

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::PROMPTS_GET, paramsWriter.TakeString(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    result = response.value();
    co_return;
}

kernel::Coroutine McpHttpClient::ping(std::expected<void, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<JsonString, McpError> response;
    co_await sendRequest(Methods::PING, EmptyObjectString(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    result = {};
    co_return;
}

async::CloseAwaitable McpHttpClient::disconnect() {
    m_initialized = false;
    m_connected = false;
    return m_httpClient->close();
}

kernel::Coroutine McpHttpClient::sendRequest(std::string method,
                                              std::optional<JsonString> params,
                                              std::expected<JsonString, McpError>& result) {
    // 构建JSON-RPC请求
    JsonRpcRequest request;
    request.id = generateRequestId();
    request.method = std::move(method);
    request.params = std::move(params);

    std::string requestBody = request.toJson();

    // 如果连接断开，重新连接
    if (!m_connected.load()) {
        auto connectResult = co_await m_httpClient->connect(m_serverUrl);
        if (!connectResult) {
            result = std::unexpected(McpError::connectionError(connectResult.error().message()));
            co_return;
        }
        m_connected = true;
    }

    // 发送POST请求
    auto session = m_httpClient->getSession();
    auto awaitable = session.post(
        m_httpClient->url().path,
        requestBody,
        "application/json",
        {
            {"Host", m_httpClient->url().host + ":" + std::to_string(m_httpClient->url().port)},
            {"Content-Type", "application/json"}
        }
    );

    // 循环等待直到完成
    while (true) {
        auto httpResult = co_await awaitable;

        if (!httpResult) {
            m_connected = false;
            result = std::unexpected(McpError::connectionError(httpResult.error().message()));
            co_return;
        }

        if (!httpResult.value()) {
            continue;
        }

        auto response = httpResult.value().value();

        // 根据响应头判断是否需要关闭连接
        if (response.header().isConnectionClose() || !response.header().isKeepAlive()) {
            m_connected = false;
        }

        // 检查HTTP状态码
        if (response.header().code() != http::HttpStatusCode::OK_200) {
            result = std::unexpected(McpError::connectionError(
                "HTTP error: " + std::to_string(static_cast<int>(response.header().code()))));
            co_return;
        }

        // 解析响应
        std::string responseBody = response.getBodyStr();
        auto parsed = parseJsonRpcResponse(responseBody);
        if (!parsed) {
            result = std::unexpected(McpError::parseError(parsed.error().details()));
            co_return;
        }

        const auto& view = parsed.value().response;
        if (view.hasError) {
            auto errorExp = JsonRpcError::fromJson(view.error);
            if (!errorExp) {
                result = std::unexpected(McpError::parseError(errorExp.error().message()));
                co_return;
            }
            const auto& error = errorExp.value();
            std::string details;
            if (error.data.has_value()) {
                details = error.data.value();
            }
            result = std::unexpected(McpError::fromJsonRpcError(
                error.code, error.message, details));
            co_return;
        }

        if (view.hasResult) {
            std::string raw;
            if (JsonHelper::GetRawJson(view.result, raw)) {
                result = std::move(raw);
            } else {
                result = std::unexpected(McpError::parseError("Failed to parse result"));
            }
        } else {
            result = EmptyObjectString();
        }

        co_return;
    }
}

int64_t McpHttpClient::generateRequestId() {
    return m_requestIdCounter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace mcp
} // namespace galay
