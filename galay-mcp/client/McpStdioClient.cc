#include "McpStdioClient.h"
#include <sstream>

namespace galay {
namespace mcp {

McpStdioClient::McpStdioClient()
    : m_initialized(false)
    , m_requestIdCounter(0)
    , m_input(&std::cin)
    , m_output(&std::cout) {
}

McpStdioClient::~McpStdioClient() {
    disconnect();
}

std::expected<void, McpError> McpStdioClient::initialize(const std::string& clientName,
                                                         const std::string& clientVersion) {
    if (m_initialized) {
        return std::unexpected(McpError::alreadyInitialized());
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

        // 发送initialized通知
        sendNotification(Methods::INITIALIZED, Json::object());

        return {};
    } catch (const std::exception& e) {
        return std::unexpected(McpError::initializationFailed(e.what()));
    }
}

std::expected<Json, McpError> McpStdioClient::callTool(const std::string& toolName,
                                                       const Json& arguments) {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
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
            return std::unexpected(McpError::toolExecutionFailed("Tool returned error"));
        }

        if (callResult.content.empty()) {
            return Json::object();
        }

        // 返回第一个内容项的文本
        if (callResult.content[0].type == ContentType::Text) {
            return Json::parse(callResult.content[0].text);
        }

        return Json::object();
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::vector<Tool>, McpError> McpStdioClient::listTools() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::TOOLS_LIST, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        std::vector<Tool> tools;
        if (result.value().contains("tools")) {
            for (const auto& item : result.value()["tools"]) {
                tools.push_back(Tool::fromJson(item));
            }
        }
        return tools;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::vector<Resource>, McpError> McpStdioClient::listResources() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::RESOURCES_LIST, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        std::vector<Resource> resources;
        if (result.value().contains("resources")) {
            for (const auto& item : result.value()["resources"]) {
                resources.push_back(Resource::fromJson(item));
            }
        }
        return resources;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::string, McpError> McpStdioClient::readResource(const std::string& uri) {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    Json params;
    params["uri"] = uri;

    auto result = sendRequest(Methods::RESOURCES_READ, params);
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        if (result.value().contains("contents") && result.value()["contents"].is_array()) {
            auto contents = result.value()["contents"];
            if (!contents.empty()) {
                Content content = Content::fromJson(contents[0]);
                if (content.type == ContentType::Text) {
                    return content.text;
                }
            }
        }
        return "";
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<std::vector<Prompt>, McpError> McpStdioClient::listPrompts() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::PROMPTS_LIST, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    try {
        std::vector<Prompt> prompts;
        if (result.value().contains("prompts")) {
            for (const auto& item : result.value()["prompts"]) {
                prompts.push_back(Prompt::fromJson(item));
            }
        }
        return prompts;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<Json, McpError> McpStdioClient::getPrompt(const std::string& name,
                                                        const Json& arguments) {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    Json params;
    params["name"] = name;
    if (!arguments.is_null()) {
        params["arguments"] = arguments;
    }

    auto result = sendRequest(Methods::PROMPTS_GET, params);
    if (!result) {
        return std::unexpected(result.error());
    }

    return result.value();
}

std::expected<void, McpError> McpStdioClient::ping() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::PING, Json::object());
    if (!result) {
        return std::unexpected(result.error());
    }

    return {};
}

void McpStdioClient::disconnect() {
    m_initialized = false;
}

bool McpStdioClient::isInitialized() const {
    return m_initialized;
}

const ServerInfo& McpStdioClient::getServerInfo() const {
    return m_serverInfo;
}

const ServerCapabilities& McpStdioClient::getServerCapabilities() const {
    return m_serverCapabilities;
}

std::expected<Json, McpError> McpStdioClient::sendRequest(const std::string& method,
                                                          const Json& params) {
    JsonRpcRequest request;
    request.id = generateRequestId();
    request.method = method;
    request.params = params;

    auto writeResult = writeMessage(request.toJson());
    if (!writeResult) {
        return std::unexpected(writeResult.error());
    }

    // 读取响应
    auto readResult = readMessage();
    if (!readResult) {
        return std::unexpected(readResult.error());
    }

    try {
        JsonRpcResponse response = JsonRpcResponse::fromJson(readResult.value());

        if (response.error.has_value()) {
            JsonRpcError error = JsonRpcError::fromJson(response.error.value());
            std::string details = error.data.has_value() ?
                                 error.data.value().dump() : "";
            return std::unexpected(McpError(
                static_cast<McpErrorCode>(error.code),
                error.message,
                details
            ));
        }

        if (response.result.has_value()) {
            return response.result.value();
        }

        return Json::object();
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<void, McpError> McpStdioClient::sendNotification(const std::string& method,
                                                               const Json& params) {
    JsonRpcNotification notification;
    notification.method = method;
    notification.params = params;

    return writeMessage(notification.toJson());
}

std::expected<Json, McpError> McpStdioClient::readMessage() {
    std::lock_guard<std::mutex> lock(m_inputMutex);

    std::string line;
    if (!std::getline(*m_input, line)) {
        return std::unexpected(McpError::readError("Failed to read from stdin"));
    }

    if (line.empty()) {
        return std::unexpected(McpError::invalidMessage("Empty message"));
    }

    try {
        Json j = Json::parse(line);
        return j;
    } catch (const std::exception& e) {
        return std::unexpected(McpError::parseError(e.what()));
    }
}

std::expected<void, McpError> McpStdioClient::writeMessage(const Json& message) {
    std::lock_guard<std::mutex> lock(m_outputMutex);

    try {
        std::string line = message.dump();
        *m_output << line << std::endl;
        m_output->flush();
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(McpError::writeError(e.what()));
    }
}

int64_t McpStdioClient::generateRequestId() {
    return ++m_requestIdCounter;
}

} // namespace mcp
} // namespace galay
