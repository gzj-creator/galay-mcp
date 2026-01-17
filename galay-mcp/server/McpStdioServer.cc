#include "McpStdioServer.h"
#include <sstream>
#include <stdexcept>
#include <mutex>

namespace galay {
namespace mcp {

McpStdioServer::McpStdioServer()
    : m_serverName("galay-mcp-server")
    , m_serverVersion("1.0.0")
    , m_running(false)
    , m_initialized(false)
    , m_input(&std::cin)
    , m_output(&std::cout) {
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

void McpStdioServer::addResource(const std::string& uri,
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

void McpStdioServer::addPrompt(const std::string& name,
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

        try {
            JsonRpcRequest request = JsonRpcRequest::fromJson(messageResult.value());
            handleRequest(request);
        } catch (const std::exception& e) {
            // 解析失败，发送错误响应
            sendError(0, ErrorCodes::PARSE_ERROR, "Parse error", e.what());
        }
    }

    m_running = false;
}

void McpStdioServer::stop() {
    m_running = false;
}

bool McpStdioServer::isRunning() const {
    return m_running;
}

void McpStdioServer::handleRequest(const JsonRpcRequest& request) {
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

void McpStdioServer::handleInitialize(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Already initialized", "");
        return;
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

        sendResponse(response);

        m_initialized = true;

        // 发送initialized通知
        sendNotification(Methods::INITIALIZED, Json::object());

    } catch (const std::exception& e) {
        sendError(request.id.value(), ErrorCodes::INVALID_PARAMS,
                 "Invalid parameters", e.what());
    }
}

void McpStdioServer::handleToolsList(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
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

    sendResponse(response);
}

void McpStdioServer::handleToolsCall(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    try {
        ToolCallParams params = ToolCallParams::fromJson(request.params);

        std::shared_lock<std::shared_mutex> lock(m_toolsMutex);

        auto it = m_tools.find(params.name);
        if (it == m_tools.end()) {
            sendError(request.id.value(), ErrorCodes::METHOD_NOT_FOUND,
                     "Tool not found", params.name);
            return;
        }

        // 调用工具处理函数
        auto result = it->second.handler(params.arguments);

        if (!result) {
            sendError(request.id.value(), result.error().toJsonRpcErrorCode(),
                     result.error().message(), result.error().details());
            return;
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

        sendResponse(response);

    } catch (const std::exception& e) {
        sendError(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                 "Internal error", e.what());
    }
}

void McpStdioServer::handleResourcesList(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
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

    sendResponse(response);
}

void McpStdioServer::handleResourcesRead(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    try {
        std::string uri = request.params["uri"];

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

        sendResponse(response);

    } catch (const std::exception& e) {
        sendError(request.id.value(), ErrorCodes::INTERNAL_ERROR,
                 "Internal error", e.what());
    }
}

void McpStdioServer::handlePromptsList(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
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

    sendResponse(response);
}

void McpStdioServer::handlePromptsGet(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    if (!m_initialized) {
        sendError(request.id.value(), ErrorCodes::INVALID_REQUEST,
                 "Not initialized", "");
        return;
    }

    try {
        std::string name = request.params["name"];
        Json arguments = request.params.contains("arguments") ?
                        request.params["arguments"] : Json::object();

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

void McpStdioServer::handlePing(const JsonRpcRequest& request) {
    if (!request.id.has_value()) {
        return;
    }

    JsonRpcResponse response;
    response.id = request.id.value();
    response.result = Json::object();

    sendResponse(response);
}

void McpStdioServer::sendResponse(const JsonRpcResponse& response) {
    Json j = response.toJson();
    writeMessage(j);
}

void McpStdioServer::sendError(int64_t id, int code, const std::string& message,
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

    sendResponse(response);
}

void McpStdioServer::sendNotification(const std::string& method, const Json& params) {
    JsonRpcNotification notification;
    notification.method = method;
    notification.params = params;

    Json j = notification.toJson();
    writeMessage(j);
}

std::expected<Json, McpError> McpStdioServer::readMessage() {
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

std::expected<void, McpError> McpStdioServer::writeMessage(const Json& message) {
    std::lock_guard<std::mutex> lock(m_outputMutex);

    try {
        std::string line = message.dump();
        *m_output << line << '\n';  // 使用 \n 替代 std::endl，避免不必要的flush
        m_output->flush();
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(McpError::writeError(e.what()));
    }
}

} // namespace mcp
} // namespace galay
