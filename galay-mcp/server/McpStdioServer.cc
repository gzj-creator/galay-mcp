#include "galay-mcp/server/McpStdioServer.h"
#include "galay-mcp/common/McpProtocolUtils.h"
#include <sstream>
#include <stdexcept>
#include <mutex>

namespace galay {
namespace mcp {

namespace {

JsonString EmptyObjectString() {
    return "{}";
}

} // namespace

McpStdioServer::McpStdioServer()
    : m_serverName("galay-mcp-server")
    , m_serverVersion("1.0.0")
    , m_running(false)
    , m_initialized(false)
    , m_input(&std::cin)
    , m_output(&std::cout) {
    m_toolsListCache = protocol::buildListResultFromMap(
        m_tools, "tools",
        [](const ToolInfo& info) -> const Tool& { return info.tool; });
    m_resourcesListCache = protocol::buildListResultFromMap(
        m_resources, "resources",
        [](const ResourceInfo& info) -> const Resource& { return info.resource; });
    m_promptsListCache = protocol::buildListResultFromMap(
        m_prompts, "prompts",
        [](const PromptInfo& info) -> const Prompt& { return info.prompt; });
}

McpStdioServer::~McpStdioServer() {
    stop();
}

void McpStdioServer::setServerInfo(const std::string& name, const std::string& version) {
    m_serverName = name;
    m_serverVersion = version;
}

void McpStdioServer::addTool(const std::string& name,
                             const std::string& description,
                             const JsonString& inputSchema,
                             McpStdioServer::ToolHandler handler) {
    std::unique_lock<std::shared_mutex> lock(m_toolsMutex);

    Tool tool;
    tool.name = name;
    tool.description = description;
    tool.inputSchema = inputSchema;

    ToolInfo info;
    info.tool = tool;
    info.handler = handler;

    m_tools[name] = info;
    m_toolsListCache = protocol::buildListResultFromMap(
        m_tools, "tools",
        [](const ToolInfo& info) -> const Tool& { return info.tool; });
}

void McpStdioServer::addResource(const std::string& uri,
                                 const std::string& name,
                                 const std::string& description,
                                 const std::string& mimeType,
                                 McpStdioServer::ResourceReader reader) {
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
    m_resourcesListCache = protocol::buildListResultFromMap(
        m_resources, "resources",
        [](const ResourceInfo& info) -> const Resource& { return info.resource; });
}

void McpStdioServer::addPrompt(const std::string& name,
                               const std::string& description,
                               const std::vector<PromptArgument>& arguments,
                               McpStdioServer::PromptGetter getter) {
    std::unique_lock<std::shared_mutex> lock(m_promptsMutex);

    Prompt prompt;
    prompt.name = name;
    prompt.description = description;
    prompt.arguments = arguments;

    PromptInfo info;
    info.prompt = prompt;
    info.getter = getter;

    m_prompts[name] = info;
    m_promptsListCache = protocol::buildListResultFromMap(
        m_prompts, "prompts",
        [](const PromptInfo& info) -> const Prompt& { return info.prompt; });
}

void McpStdioServer::run() {
    m_running = true;

    while (m_running) {
        auto messageResult = readMessage();
        if (!messageResult) {
            // 读取失败，可能是EOF或错误
            if (m_input->eof()) {
                break;
            }
            continue;
        }

        auto parsed = parseJsonRpcRequest(messageResult.value());
        if (!parsed) {
            sendError(0, ErrorCodes::PARSE_ERROR, "Parse error", parsed.error().details());
            continue;
        }

        handleRequest(parsed.value().request);
    }

    m_running = false;
}

void McpStdioServer::stop() {
    m_running = false;
}

bool McpStdioServer::isRunning() const {
    return m_running;
}

void McpStdioServer::handleRequest(const JsonRpcRequestView& request) {
    const std::string& method = request.method;

    if (method == Methods::INITIALIZE) {
        handleInitialize(request);
    } else if (method == Methods::TOOLS_LIST) {
        handleToolsList(request);
    } else if (method == Methods::TOOLS_CALL) {
        handleToolsCall(request);
    } else if (method == Methods::RESOURCES_LIST) {
        handleResourcesList(request);
    } else if (method == Methods::RESOURCES_READ) {
        handleResourcesRead(request);
    } else if (method == Methods::PROMPTS_LIST) {
        handlePromptsList(request);
    } else if (method == Methods::PROMPTS_GET) {
        handlePromptsGet(request);
    } else if (method == Methods::PING) {
        handlePing(request);
    } else {
        if (request.id.has_value()) {
            sendError(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                     "Method not found", method);
        }
    }
}

void McpStdioServer::handleInitialize(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Already initialized", "");
        return;
    }

    if (!request.hasParams) {
        sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                 "Invalid parameters", "Missing params");
        return;
    }

    auto paramsExp = InitializeParams::fromJson(request.params);
    if (!paramsExp) {
        sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                 "Invalid parameters", paramsExp.error().message());
        return;
    }

    // 构建响应
    JsonString result = protocol::buildInitializeResult(
        m_serverName,
        m_serverVersion,
        !m_tools.empty(),
        !m_resources.empty(),
        !m_prompts.empty());

    JsonRpcResponse response = protocol::makeResultResponse(request.id.value(), result);

    sendResponse(response);

    m_initialized = true;

    // 发送initialized通知
    sendNotification(Methods::INITIALIZED, EmptyObjectString());
}

void McpStdioServer::handleToolsList(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    std::shared_lock<std::shared_mutex> lock(m_toolsMutex);

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), m_toolsListCache);

    sendResponse(response);
}

void McpStdioServer::handleToolsCall(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    try {
        if (!request.hasParams) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Missing params");
            return;
        }

        JsonObject paramsObj;
        if (!JsonHelper::GetObject(request.params, paramsObj)) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Params must be object");
            return;
        }

        std::string toolName;
        if (!JsonHelper::GetString(paramsObj, "name", toolName)) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Missing tool name");
            return;
        }

        std::shared_lock<std::shared_mutex> lock(m_toolsMutex);

        auto it = m_tools.find(toolName);
        if (it == m_tools.end()) {
            sendError(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                     "Tool not found", toolName);
            return;
        }

        JsonElement arguments = JsonHelper::EmptyObject();
        JsonElement argsElement;
        if (JsonHelper::GetElement(paramsObj, "arguments", argsElement)) {
            arguments = argsElement;
        }

        // 调用工具处理函数
        auto result = it->second.handler(arguments);

        if (!result) {
            sendError(request.id.value(), result.error().toJsonRpcErrorCode(),
                     result.error().message(), result.error().details());
            return;
        }

        // 构建响应
        ToolCallResult callResult;
        Content content;
        content.type = ContentType::Text;
        content.text = result.value();
        callResult.content.push_back(content);

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = callResult.toJson();

        sendResponse(response);

    } catch (const std::exception& e) {
        sendError(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                 "Internal error", e.what());
    }
}

void McpStdioServer::handleResourcesList(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    std::shared_lock<std::shared_mutex> lock(m_resourcesMutex);

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), m_resourcesListCache);

    sendResponse(response);
}

void McpStdioServer::handleResourcesRead(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    try {
        if (!request.hasParams) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Missing params");
            return;
        }

        JsonObject paramsObj;
        if (!JsonHelper::GetObject(request.params, paramsObj)) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Params must be object");
            return;
        }

        std::string uri;
        if (!JsonHelper::GetString(paramsObj, "uri", uri)) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Missing uri");
            return;
        }

        std::shared_lock<std::shared_mutex> lock(m_resourcesMutex);

        auto it = m_resources.find(uri);
        if (it == m_resources.end()) {
            sendError(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                     "Resource not found", uri);
            return;
        }

        // 调用资源读取函数
        auto result = it->second.reader(uri);

        if (!result) {
            sendError(request.id.value(), result.error().toJsonRpcErrorCode(),
                     result.error().message(), result.error().details());
            return;
        }

        // 构建响应
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

        sendResponse(response);

    } catch (const std::exception& e) {
        sendError(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                 "Internal error", e.what());
    }
}

void McpStdioServer::handlePromptsList(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    std::shared_lock<std::shared_mutex> lock(m_promptsMutex);

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), m_promptsListCache);

    sendResponse(response);
}

void McpStdioServer::handlePromptsGet(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    try {
        if (!request.hasParams) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Missing params");
            return;
        }

        JsonObject paramsObj;
        if (!JsonHelper::GetObject(request.params, paramsObj)) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Params must be object");
            return;
        }

        std::string name;
        if (!JsonHelper::GetString(paramsObj, "name", name)) {
            sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                     "Invalid parameters", "Missing prompt name");
            return;
        }

        JsonElement arguments = JsonHelper::EmptyObject();
        JsonElement argsElement;
        if (JsonHelper::GetElement(paramsObj, "arguments", argsElement)) {
            arguments = argsElement;
        }

        std::shared_lock<std::shared_mutex> lock(m_promptsMutex);

        auto it = m_prompts.find(name);
        if (it == m_prompts.end()) {
            sendError(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                     "Prompt not found", name);
            return;
        }

        // 调用提示获取函数
        auto result = it->second.getter(name, arguments);

        if (!result) {
            sendError(request.id.value(), result.error().toJsonRpcErrorCode(),
                     result.error().message(), result.error().details());
            return;
        }

        JsonRpcResponse response;
        response.id = request.id.value();
        response.result = result.value();

        sendResponse(response);

    } catch (const std::exception& e) {
        sendError(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                 "Internal error", e.what());
    }
}

void McpStdioServer::handlePing(const JsonRpcRequestView& request) {
    if (!request.id.has_value()) {
        return;
    }

    JsonRpcResponse response = protocol::makeResultResponse(
        request.id.value(), EmptyObjectString());

    sendResponse(response);
}

void McpStdioServer::sendResponse(const JsonRpcResponse& response) {
    writeMessage(response.toJson());
}

void McpStdioServer::sendError(int64_t id, int code, const std::string& message,
                               const std::string& details) {
    sendResponse(protocol::makeErrorResponse(id, code, message, details));
}

void McpStdioServer::sendNotification(const std::string& method, const JsonString& params) {
    JsonRpcNotification notification;
    notification.method = method;
    notification.params = params;

    writeMessage(notification.toJson());
}

std::expected<std::string, McpError> McpStdioServer::readMessage() {
    std::string line;
    if (!std::getline(*m_input, line)) {
        return std::unexpected(McpError::readError("Failed to read from stdin"));
    }

    if (line.empty()) {
        return std::unexpected(McpError::invalidMessage("Empty message"));
    }

    return line;
}

std::expected<void, McpError> McpStdioServer::writeMessage(const JsonString& message) {
    std::lock_guard<std::mutex> lock(m_outputMutex);

    try {
        *m_output << message << '\n';
        m_output->flush();
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(McpError::writeError(e.what()));
    }
}

} // namespace mcp
} // namespace galay
