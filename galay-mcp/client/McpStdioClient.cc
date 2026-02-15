#include "galay-mcp/client/McpStdioClient.h"
#include <sstream>

namespace galay {
namespace mcp {

namespace {

JsonString EmptyObjectString() {
    return "{}";
}

} // namespace

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
    params.capabilities = EmptyObjectString();

    auto result = sendRequest(Methods::INITIALIZE, params.toJson());
    if (!result) {
        return std::unexpected(result.error());
    }

    auto docExp = JsonDocument::Parse(result.value());
    if (!docExp) {
        return std::unexpected(McpError::initializationFailed(docExp.error().details()));
    }

    auto initExp = InitializeResult::fromJson(docExp.value().Root());
    if (!initExp) {
        return std::unexpected(McpError::initializationFailed(initExp.error().message()));
    }

    m_serverInfo = initExp.value().serverInfo;
    m_serverCapabilities = initExp.value().capabilities;
    m_initialized = true;

    // 发送initialized通知
    sendNotification(Methods::INITIALIZED, EmptyObjectString());

    return {};
}

std::expected<JsonString, McpError> McpStdioClient::callTool(const std::string& toolName,
                                                             const JsonString& arguments) {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    ToolCallParams params;
    params.name = toolName;
    params.arguments = arguments.empty() ? EmptyObjectString() : arguments;

    auto result = sendRequest(Methods::TOOLS_CALL, params.toJson());
    if (!result) {
        return std::unexpected(result.error());
    }

    auto docExp = JsonDocument::Parse(result.value());
    if (!docExp) {
        return std::unexpected(McpError::parseError(docExp.error().details()));
    }

    auto callExp = ToolCallResult::fromJson(docExp.value().Root());
    if (!callExp) {
        return std::unexpected(McpError::parseError(callExp.error().message()));
    }

    const auto& callResult = callExp.value();
    if (callResult.isError) {
        return std::unexpected(McpError::toolExecutionFailed("Tool returned error"));
    }

    if (callResult.content.empty()) {
        return EmptyObjectString();
    }

    if (callResult.content[0].type == ContentType::Text) {
        return callResult.content[0].text;
    }

    return EmptyObjectString();
}

std::expected<std::vector<Tool>, McpError> McpStdioClient::listTools() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::TOOLS_LIST, EmptyObjectString());
    if (!result) {
        return std::unexpected(result.error());
    }

    auto docExp = JsonDocument::Parse(result.value());
    if (!docExp) {
        return std::unexpected(McpError::parseError(docExp.error().details()));
    }

    std::vector<Tool> tools;
    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "tools", arr)) {
            for (auto item : arr) {
                auto toolExp = Tool::fromJson(item);
                if (!toolExp) {
                    return std::unexpected(McpError::parseError(toolExp.error().message()));
                }
                tools.push_back(std::move(toolExp.value()));
            }
        }
    }
    return tools;
}

std::expected<std::vector<Resource>, McpError> McpStdioClient::listResources() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::RESOURCES_LIST, EmptyObjectString());
    if (!result) {
        return std::unexpected(result.error());
    }

    auto docExp = JsonDocument::Parse(result.value());
    if (!docExp) {
        return std::unexpected(McpError::parseError(docExp.error().details()));
    }

    std::vector<Resource> resources;
    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "resources", arr)) {
            for (auto item : arr) {
                auto resExp = Resource::fromJson(item);
                if (!resExp) {
                    return std::unexpected(McpError::parseError(resExp.error().message()));
                }
                resources.push_back(std::move(resExp.value()));
            }
        }
    }
    return resources;
}

std::expected<std::string, McpError> McpStdioClient::readResource(const std::string& uri) {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    JsonWriter paramsWriter;
    paramsWriter.StartObject();
    paramsWriter.Key("uri");
    paramsWriter.String(uri);
    paramsWriter.EndObject();

    auto result = sendRequest(Methods::RESOURCES_READ, paramsWriter.TakeString());
    if (!result) {
        return std::unexpected(result.error());
    }

    auto docExp = JsonDocument::Parse(result.value());
    if (!docExp) {
        return std::unexpected(McpError::parseError(docExp.error().details()));
    }

    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "contents", arr)) {
            for (auto item : arr) {
                auto contentExp = Content::fromJson(item);
                if (!contentExp) {
                    return std::unexpected(McpError::parseError(contentExp.error().message()));
                }
                if (contentExp.value().type == ContentType::Text) {
                    return contentExp.value().text;
                }
            }
        }
    }
    return "";
}

std::expected<std::vector<Prompt>, McpError> McpStdioClient::listPrompts() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::PROMPTS_LIST, EmptyObjectString());
    if (!result) {
        return std::unexpected(result.error());
    }

    auto docExp = JsonDocument::Parse(result.value());
    if (!docExp) {
        return std::unexpected(McpError::parseError(docExp.error().details()));
    }

    std::vector<Prompt> prompts;
    JsonObject obj;
    if (JsonHelper::GetObject(docExp.value().Root(), obj)) {
        JsonArray arr;
        if (JsonHelper::GetArray(obj, "prompts", arr)) {
            for (auto item : arr) {
                auto promptExp = Prompt::fromJson(item);
                if (!promptExp) {
                    return std::unexpected(McpError::parseError(promptExp.error().message()));
                }
                prompts.push_back(std::move(promptExp.value()));
            }
        }
    }
    return prompts;
}

std::expected<JsonString, McpError> McpStdioClient::getPrompt(const std::string& name,
                                                              const JsonString& arguments) {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    JsonWriter paramsWriter;
    paramsWriter.StartObject();
    paramsWriter.Key("name");
    paramsWriter.String(name);
    if (!arguments.empty()) {
        paramsWriter.Key("arguments");
        paramsWriter.Raw(arguments);
    }
    paramsWriter.EndObject();

    auto result = sendRequest(Methods::PROMPTS_GET, paramsWriter.TakeString());
    if (!result) {
        return std::unexpected(result.error());
    }

    return result.value();
}

std::expected<void, McpError> McpStdioClient::ping() {
    if (!m_initialized) {
        return std::unexpected(McpError::notInitialized());
    }

    auto result = sendRequest(Methods::PING, EmptyObjectString());
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

std::expected<JsonString, McpError> McpStdioClient::sendRequest(const std::string& method,
                                                                const std::optional<JsonString>& params) {
    JsonRpcRequest request;
    request.id = generateRequestId();
    request.method = method;
    request.params = params;

    auto writeResult = writeMessage(request.toJson());
    if (!writeResult) {
        return std::unexpected(writeResult.error());
    }

    // 读取响应（跳过通知）
    while (true) {
        auto readResult = readMessage();
        if (!readResult) {
            return std::unexpected(readResult.error());
        }

        auto docExp = JsonDocument::Parse(readResult.value());
        if (!docExp) {
            return std::unexpected(McpError::parseError(docExp.error().details()));
        }

        JsonObject obj;
        if (!JsonHelper::GetObject(docExp.value().Root(), obj)) {
            return std::unexpected(McpError::invalidResponse("Invalid response object"));
        }

        auto idVal = obj["id"];
        if (idVal.error() || idVal.is_null()) {
            // 通知消息，忽略
            continue;
        }
        if (!idVal.is_int64()) {
            return std::unexpected(McpError::invalidResponse("Invalid response id"));
        }

        auto errorVal = obj["error"];
        if (!errorVal.error() && !errorVal.is_null()) {
            auto errExp = JsonRpcError::fromJson(errorVal.value());
            if (!errExp) {
                return std::unexpected(McpError::parseError(errExp.error().message()));
            }
            std::string details;
            if (errExp.value().data.has_value()) {
                details = errExp.value().data.value();
            }
            return std::unexpected(McpError::fromJsonRpcError(
                errExp.value().code, errExp.value().message, details));
        }

        auto resultVal = obj["result"];
        if (!resultVal.error() && !resultVal.is_null()) {
            std::string raw;
            if (!JsonHelper::GetRawJson(resultVal.value(), raw)) {
                return std::unexpected(McpError::parseError("Failed to parse result"));
            }
            return raw;
        }

        return EmptyObjectString();
    }
}

std::expected<void, McpError> McpStdioClient::sendNotification(const std::string& method,
                                                               const std::optional<JsonString>& params) {
    JsonRpcNotification notification;
    notification.method = method;
    notification.params = params;

    return writeMessage(notification.toJson());
}

std::expected<std::string, McpError> McpStdioClient::readMessage() {
    std::lock_guard<std::mutex> lock(m_inputMutex);

    std::string line;
    if (!std::getline(*m_input, line)) {
        return std::unexpected(McpError::readError("Failed to read from stdin"));
    }

    if (line.empty()) {
        return std::unexpected(McpError::invalidMessage("Empty message"));
    }

    return line;
}

std::expected<void, McpError> McpStdioClient::writeMessage(const JsonString& message) {
    std::lock_guard<std::mutex> lock(m_outputMutex);

    try {
        *m_output << message << '\n';
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
