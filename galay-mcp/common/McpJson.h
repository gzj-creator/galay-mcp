#ifndef GALAY_MCP_COMMON_MCPJSON_H
#define GALAY_MCP_COMMON_MCPJSON_H

#include "galay-mcp/common/McpError.h"
#include <simdjson.h>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace galay {
namespace mcp {

using JsonString = std::string;
using JsonElement = simdjson::dom::element;
using JsonObject = simdjson::dom::object;
using JsonArray = simdjson::dom::array;

class JsonDocument {
public:
    JsonDocument() = default;

    static std::expected<JsonDocument, McpError> Parse(std::string_view json);

    const JsonElement& Root() const { return m_root; }
    JsonElement& Root() { return m_root; }
    std::string_view Raw() const { return std::string_view(m_buffer.data(), m_buffer.size()); }

private:
    std::unique_ptr<simdjson::dom::parser> m_parser;
    simdjson::padded_string m_buffer;
    JsonElement m_root;
};

class JsonWriter {
public:
    void StartObject();
    void EndObject();
    void StartArray();
    void EndArray();
    void Key(const std::string& key);
    void String(const std::string& value);
    void Number(int64_t value);
    void Number(uint64_t value);
    void Number(double value);
    void Bool(bool value);
    void Null();
    void Raw(const std::string& json);
    std::string TakeString();

private:
    enum class ContextType {
        Object,
        Array
    };

    struct Context {
        ContextType type;
        bool first = true;
        bool expectValue = false; // for object after Key()
    };

    void WriteValuePrefix();
    void WriteCommaIfNeeded();
    static void AppendEscaped(std::string& out, const std::string& value);

    std::string m_out;
    std::vector<Context> m_stack;
};

class JsonHelper {
public:
    static bool GetObject(const JsonElement& element, JsonObject& out);
    static bool GetArray(const JsonElement& element, JsonArray& out);
    static bool GetStringValue(const JsonElement& element, std::string& out);
    static bool GetRawJson(const JsonElement& element, std::string& out);

    static bool GetString(const JsonObject& obj, const char* key, std::string& out);
    static bool GetInt64(const JsonObject& obj, const char* key, int64_t& out);
    static bool GetBool(const JsonObject& obj, const char* key, bool& out);
    static bool GetElement(const JsonObject& obj, const char* key, JsonElement& out);
    static bool GetObject(const JsonObject& obj, const char* key, JsonObject& out);
    static bool GetArray(const JsonObject& obj, const char* key, JsonArray& out);

    static const JsonElement& EmptyObject();
};

} // namespace mcp
} // namespace galay

#endif // GALAY_MCP_COMMON_MCPJSON_H
