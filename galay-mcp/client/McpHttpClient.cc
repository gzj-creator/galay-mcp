#include "McpHttpClient.h"

namespace galay {
namespace mcp {

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
    params.capabilities = Json::object();

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::INITIALIZE, params.toJson(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    try {
        InitializeResult initResult = InitializeResult::fromJson(response.value());
        m_serverInfo = initResult.serverInfo;
        m_serverCapabilities = initResult.capabilities;
        m_initialized = true;
        m_connected = true;
        result = {};
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::initializationFailed(e.what()));
    }

    co_return;
}

kernel::Coroutine McpHttpClient::callTool(std::string toolName,
                                           Json arguments,
                                           std::expected<Json, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    ToolCallParams params;
    params.name = std::move(toolName);
    params.arguments = std::move(arguments);

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::TOOLS_CALL, params.toJson(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    try {
        ToolCallResult callResult = ToolCallResult::fromJson(response.value());
        if (callResult.isError) {
            result = std::unexpected(McpError::toolExecutionFailed("Tool returned error"));
            co_return;
        }
        if (callResult.content.empty()) {
            result = Json::object();
            co_return;
        }
        if (callResult.content[0].type == ContentType::Text) {
            result = Json::parse(callResult.content[0].text);
        } else {
            result = Json::object();
        }
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::parseError(e.what()));
    }

    co_return;
}

kernel::Coroutine McpHttpClient::listTools(std::expected<std::vector<Tool>, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::TOOLS_LIST, Json::object(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    try {
        std::vector<Tool> tools;
        if (response.value().contains("tools")) {
            for (const auto& item : response.value()["tools"]) {
                tools.push_back(Tool::fromJson(item));
            }
        }
        result = std::move(tools);
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::parseError(e.what()));
    }

    co_return;
}

kernel::Coroutine McpHttpClient::listResources(std::expected<std::vector<Resource>, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::RESOURCES_LIST, Json::object(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    try {
        std::vector<Resource> resources;
        if (response.value().contains("resources")) {
            for (const auto& item : response.value()["resources"]) {
                resources.push_back(Resource::fromJson(item));
            }
        }
        result = std::move(resources);
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::parseError(e.what()));
    }

    co_return;
}

kernel::Coroutine McpHttpClient::readResource(std::string uri,
                                               std::expected<std::string, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    Json params;
    params["uri"] = std::move(uri);

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::RESOURCES_READ, params, response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    try {
        if (response.value().contains("contents") && response.value()["contents"].is_array()) {
            auto contents = response.value()["contents"];
            if (!contents.empty()) {
                Content content = Content::fromJson(contents[0]);
                if (content.type == ContentType::Text) {
                    result = content.text;
                    co_return;
                }
            }
        }
        result = "";
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::parseError(e.what()));
    }

    co_return;
}

kernel::Coroutine McpHttpClient::listPrompts(std::expected<std::vector<Prompt>, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::PROMPTS_LIST, Json::object(), response).wait();

    if (!response) {
        result = std::unexpected(response.error());
        co_return;
    }

    try {
        std::vector<Prompt> prompts;
        if (response.value().contains("prompts")) {
            for (const auto& item : response.value()["prompts"]) {
                prompts.push_back(Prompt::fromJson(item));
            }
        }
        result = std::move(prompts);
    } catch (const std::exception& e) {
        result = std::unexpected(McpError::parseError(e.what()));
    }

    co_return;
}

kernel::Coroutine McpHttpClient::getPrompt(std::string name,
                                            Json arguments,
                                            std::expected<Json, McpError>& result) {
    if (!m_initialized) {
        result = std::unexpected(McpError::notInitialized());
        co_return;
    }

    Json params;
    params["name"] = std::move(name);
    if (!arguments.is_null()) {
        params["arguments"] = std::move(arguments);
    }

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::PROMPTS_GET, params, response).wait();

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

    std::expected<Json, McpError> response;
    co_await sendRequest(Methods::PING, Json::object(), response).wait();

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
                                              Json params,
                                              std::expected<Json, McpError>& result) {
    // 构建JSON-RPC请求
    JsonRpcRequest request;
    request.id = generateRequestId();
    request.method = std::move(method);
    request.params = std::move(params);

    std::string requestBody = request.toJson().dump();

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
    auto& awaitable = m_httpClient->post(
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
        try {
            std::string responseBody = response.getBodyStr();
            Json responseJson = Json::parse(responseBody);
            JsonRpcResponse rpcResponse = JsonRpcResponse::fromJson(responseJson);

            if (rpcResponse.error.has_value()) {
                JsonRpcError error = JsonRpcError::fromJson(rpcResponse.error.value());
                result = std::unexpected(McpError::fromJsonRpcError(
                    error.code, error.message,
                    error.data.has_value() ? error.data.value().dump() : ""));
                co_return;
            }

            if (rpcResponse.result.has_value()) {
                result = rpcResponse.result.value();
            } else {
                result = Json::object();
            }
        } catch (const std::exception& e) {
            result = std::unexpected(McpError::parseError(e.what()));
        }

        co_return;
    }
}

int64_t McpHttpClient::generateRequestId() {
    return m_requestIdCounter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace mcp
} // namespace galay
