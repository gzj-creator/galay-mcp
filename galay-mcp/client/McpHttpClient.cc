#include "McpHttpClient.h"
#include <sstream>
#include <stdexcept>
#include <future>
#include <chrono>
#include <iostream>

namespace galay {
namespace mcp {

McpHttpClient::McpHttpClient(kernel::Runtime& runtime)
    : m_runtime(runtime)
    , m_httpClient(std::make_unique<http::HttpClient>())
    , m_connected(false)
    , m_initialized(false)
    , m_requestIdCounter(0) {
}

McpHttpClient::~McpHttpClient() {
    disconnect();
}

std::expected<void, McpError> McpHttpClient::connect(const std::string& url) {
    if (m_connected) {
        return std::unexpected(McpError::connectionError("Already connected"));
    }

    // 保存URL的副本
    m_serverUrl = url;

    // 使用协程连接到服务器
    std::promise<std::expected<void, McpError>> promise;
    auto future = promise.get_future();

    auto* scheduler = m_runtime.getNextIOScheduler();
    if (!scheduler) {
        return std::unexpected(McpError::connectionError("No IO scheduler available"));
    }

    scheduler->spawn([this, &promise]() -> kernel::Coroutine {
        auto result = co_await m_httpClient->connect(m_serverUrl);
        if (!result) {
            promise.set_value(std::unexpected(
                McpError::connectionError(result.error().message())));
            co_return;
        }

        m_connected = true;
        promise.set_value({});
        co_return;
    }());

    // 等待连接完成
    auto status = future.wait_for(std::chrono::seconds(10));
    if (status == std::future_status::timeout) {
        return std::unexpected(McpError::connectionError("Connection timeout"));
    }

    return future.get();
}

std::expected<void, McpError> McpHttpClient::initialize(const std::string& clientName,
                                                         const std::string& clientVersion) {
    if (!m_connected) {
        return std::unexpected(McpError::connectionError("Not connected"));
    }

    if (m_initialized) {
        return std::unexpected(McpError::invalidRequest("Already initialized"));
    }

    m_clientName = clientName;
    m_clientVersion = clientVersion;

    // 构建初始化请求
    InitializeParams params;
    params.protocolVersion = MCP_VERSION;
    params.clientInfo.name = clientName;
    params.clientInfo.version = clientVersion;
    params.capabilities = Json::object();

    auto result = sendRequest(Methods::INITIALIZE, params.toJson());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        InitializeResult initResult = InitializeResult::fromJson(result.value());
        m_serverInfo = initResult.serverInfo;
        m_serverCapabilities = initResult.capabilities;
        m_initialized = true;
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<Json, McpError> McpHttpClient::callTool(const std::string& toolName,
                                                       const Json& arguments) {
    if (!m_initialized) {
        return std::unexpected(McpError::invalidRequest("Not initialized"));
    }

    ToolCallParams params;
    params.name = toolName;
    params.arguments = arguments;

    auto result = sendRequest(Methods::TOOLS_CALL, params.toJson());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        ToolCallResult callResult = ToolCallResult::fromJson(result.value());
        if (callResult.isError) {
            return std::unexpected(McpError::toolError("Tool execution failed"));
        }

        if (callResult.content.empty()) {
            return Json::object();
        }

        // 返回第一个内容项的文本
        return Json::parse(callResult.content[0].text);
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::vector<Tool>, McpError> McpHttpClient::listTools() {
    if (!m_initialized) {
        return std::unexpected(McpError::invalidRequest("Not initialized"));
    }

    auto result = sendRequest(Methods::TOOLS_LIST, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        std::vector<Tool> tools;
        Json toolsArray = result.value()["tools"];
        for (const auto& toolJson : toolsArray) {
            tools.push_back(Tool::fromJson(toolJson));
        }
        return tools;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::vector<Resource>, McpError> McpHttpClient::listResources() {
    if (!m_initialized) {
        return std::unexpected(McpError::invalidRequest("Not initialized"));
    }

    auto result = sendRequest(Methods::RESOURCES_LIST, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        std::vector<Resource> resources;
        Json resourcesArray = result.value()["resources"];
        for (const auto& resourceJson : resourcesArray) {
            resources.push_back(Resource::fromJson(resourceJson));
        }
        return resources;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::string, McpError> McpHttpClient::readResource(const std::string& uri) {
    if (!m_initialized) {
        return std::unexpected(McpError::invalidRequest("Not initialized"));
    }

    Json params;
    params["uri"] = uri;

    auto result = sendRequest(Methods::RESOURCES_READ, params);
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        Json contents = result.value()["contents"];
        if (contents.empty()) {
            return "";
        }

        Content content = Content::fromJson(contents[0]);
        return content.text;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::vector<Prompt>, McpError> McpHttpClient::listPrompts() {
    if (!m_initialized) {
        return std::unexpected(McpError::invalidRequest("Not initialized"));
    }

    auto result = sendRequest(Methods::PROMPTS_LIST, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        std::vector<Prompt> prompts;
        Json promptsArray = result.value()["prompts"];
        for (const auto& promptJson : promptsArray) {
            prompts.push_back(Prompt::fromJson(promptJson));
        }
        return prompts;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<Json, McpError> McpHttpClient::getPrompt(const std::string& name,
                                                        const Json& arguments) {
    if (!m_initialized) {
        return std::unexpected(McpError::invalidRequest("Not initialized"));
    }

    Json params;
    params["name"] = name;
    if (!arguments.empty()) {
        params["arguments"] = arguments;
    }

    return sendRequest(Methods::PROMPTS_GET, params);
}

std::expected<void, McpError> McpHttpClient::ping() {
    if (!m_connected) {
        return std::unexpected(McpError::connectionError("Not connected"));
    }

    auto result = sendRequest(Methods::PING, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    return {};
}

void McpHttpClient::disconnect() {
    if (!m_connected) {
        return;
    }

    // 使用协程关闭连接
    auto* scheduler = m_runtime.getNextIOScheduler();
    if (scheduler) {
        scheduler->spawn([this]() -> kernel::Coroutine {
            co_await m_httpClient->close();
            co_return;
        }());
    }

    m_connected = false;
    m_initialized = false;
}

bool McpHttpClient::isConnected() const {
    return m_connected;
}

bool McpHttpClient::isInitialized() const {
    return m_initialized;
}

const ServerInfo& McpHttpClient::getServerInfo() const {
    return m_serverInfo;
}

const ServerCapabilities& McpHttpClient::getServerCapabilities() const {
    return m_serverCapabilities;
}

std::expected<Json, McpError> McpHttpClient::sendRequest(const std::string& method,
                                                          const Json& params) {
    std::lock_guard<std::mutex> lock(m_requestMutex);

    // 构建JSON-RPC请求
    JsonRpcRequest request;
    request.id = generateRequestId();
    request.method = method;
    request.params = params;

    Json requestJson = request.toJson();
    std::string requestBody = requestJson.dump();

    // 使用协程发送请求
    std::promise<std::expected<Json, McpError>> promise;
    auto future = promise.get_future();

    auto* scheduler = m_runtime.getNextIOScheduler();
    if (!scheduler) {
        return std::unexpected(McpError::connectionError("No IO scheduler available"));
    }

    scheduler->spawn([this, requestBody, &promise]() -> kernel::Coroutine {
        // 获取awaitable引用，然后在循环中重复co_await
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
            auto result = co_await awaitable;

            if (!result) {
                promise.set_value(std::unexpected(
                    McpError::connectionError(result.error().message())));
                co_return;
            }

            if (!result.value()) {
                // 请求未完成，继续等待
                continue;
            }

            auto response = result.value().value();

            // 检查HTTP状态码
            if (response.header().code() != http::HttpStatusCode::OK_200) {
                promise.set_value(std::unexpected(
                    McpError::connectionError("HTTP error: " +
                        std::to_string(static_cast<int>(response.header().code())))));
                co_return;
            }

            // 解析响应
            try {
                std::string responseBody = response.getBodyStr();
                Json responseJson = Json::parse(responseBody);

                JsonRpcResponse rpcResponse = JsonRpcResponse::fromJson(responseJson);

                if (rpcResponse.error.has_value()) {
                    JsonRpcError error = JsonRpcError::fromJson(rpcResponse.error.value());
                    promise.set_value(std::unexpected(
                        McpError::fromJsonRpcError(error.code, error.message,
                            error.data.has_value() ? error.data.value().dump() : "")));
                    co_return;
                }

                if (!rpcResponse.result.has_value()) {
                    promise.set_value(std::unexpected(
                        McpError::invalidResponse("No result in response")));
                    co_return;
                }

                promise.set_value(rpcResponse.result.value());
            } catch (const std::exception& e) {
                promise.set_value(std::unexpected(
                    McpError::parseError(e.what())));
            }

            co_return;
        }
    }());

    // 等待响应
    auto status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
        return std::unexpected(McpError::connectionError("Request timeout"));
    }

    return future.get();
}

int64_t McpHttpClient::generateRequestId() {
    return m_requestIdCounter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace mcp
} // namespace galay
