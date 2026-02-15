#ifndef GALAY_MCP_COMMON_MCPSCHEMABUILDER_H
#define GALAY_MCP_COMMON_MCPSCHEMABUILDER_H

#include "galay-mcp/common/McpBase.h"
#include "galay-mcp/common/McpJson.h"
#include <string>
#include <vector>

namespace galay {
namespace mcp {

/**
 * @brief JSON Schema 构建器
 *
 * 提供链式调用方法来简化 JSON Schema 的构建
 */
class SchemaBuilder {
public:
    SchemaBuilder() = default;

    /**
     * @brief 添加字符串属性
     */
    SchemaBuilder& addString(const std::string& name,
                             const std::string& description,
                             bool required = false) {
        Property prop;
        prop.kind = PropertyKind::String;
        prop.name = name;
        prop.description = description;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 添加数字属性
     */
    SchemaBuilder& addNumber(const std::string& name,
                             const std::string& description,
                             bool required = false) {
        Property prop;
        prop.kind = PropertyKind::Number;
        prop.name = name;
        prop.description = description;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 添加整数属性
     */
    SchemaBuilder& addInteger(const std::string& name,
                              const std::string& description,
                              bool required = false) {
        Property prop;
        prop.kind = PropertyKind::Integer;
        prop.name = name;
        prop.description = description;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 添加布尔属性
     */
    SchemaBuilder& addBoolean(const std::string& name,
                              const std::string& description,
                              bool required = false) {
        Property prop;
        prop.kind = PropertyKind::Boolean;
        prop.name = name;
        prop.description = description;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 添加数组属性
     */
    SchemaBuilder& addArray(const std::string& name,
                            const std::string& description,
                            const std::string& itemType = "string",
                            bool required = false) {
        Property prop;
        prop.kind = PropertyKind::Array;
        prop.name = name;
        prop.description = description;
        prop.itemType = itemType;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 添加对象属性（使用已有 Schema JSON）
     */
    SchemaBuilder& addObject(const std::string& name,
                             const std::string& description,
                             const JsonString& objectSchema,
                             bool required = false) {
        Property prop;
        prop.kind = PropertyKind::Object;
        prop.name = name;
        prop.description = description;
        prop.objectSchema = objectSchema;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 添加对象属性（使用 SchemaBuilder）
     */
    SchemaBuilder& addObject(const std::string& name,
                             const std::string& description,
                             const SchemaBuilder& objectSchema,
                             bool required = false) {
        return addObject(name, description, objectSchema.build(), required);
    }

    /**
     * @brief 添加枚举属性
     */
    SchemaBuilder& addEnum(const std::string& name,
                           const std::string& description,
                           const std::vector<std::string>& enumValues,
                           bool required = false) {
        Property prop;
        prop.kind = PropertyKind::Enum;
        prop.name = name;
        prop.description = description;
        prop.enumValues = enumValues;
        prop.required = required;
        m_properties.push_back(std::move(prop));
        return *this;
    }

    /**
     * @brief 构建最终的 Schema
     * @return JSON Schema 字符串
     */
    JsonString build() const {
        JsonWriter writer;
        writer.StartObject();
        writer.Key("type");
        writer.String("object");
        writer.Key("properties");
        writer.StartObject();
        for (const auto& prop : m_properties) {
            writer.Key(prop.name);
            writeProperty(writer, prop);
        }
        writer.EndObject();

        bool hasRequired = false;
        for (const auto& prop : m_properties) {
            if (prop.required) {
                hasRequired = true;
                break;
            }
        }
        if (hasRequired) {
            writer.Key("required");
            writer.StartArray();
            for (const auto& prop : m_properties) {
                if (prop.required) {
                    writer.String(prop.name);
                }
            }
            writer.EndArray();
        }
        writer.EndObject();
        return writer.TakeString();
    }

private:
    enum class PropertyKind {
        String,
        Number,
        Integer,
        Boolean,
        Array,
        Object,
        Enum
    };

    struct Property {
        PropertyKind kind{PropertyKind::String};
        std::string name;
        std::string description;
        bool required{false};
        std::string itemType;
        std::vector<std::string> enumValues;
        JsonString objectSchema;
    };

    static void writeProperty(JsonWriter& writer, const Property& prop) {
        if (prop.kind == PropertyKind::Object && !prop.objectSchema.empty()) {
            if (prop.description.empty()) {
                writer.Raw(prop.objectSchema);
                return;
            }

            auto parsed = JsonDocument::Parse(prop.objectSchema);
            if (!parsed) {
                writer.Raw(prop.objectSchema);
                return;
            }

            JsonObject obj;
            if (!JsonHelper::GetObject(parsed.value().Root(), obj)) {
                writer.Raw(prop.objectSchema);
                return;
            }

            JsonWriter merged;
            merged.StartObject();
            merged.Key("description");
            merged.String(prop.description);
            for (auto field : obj) {
                std::string raw;
                if (JsonHelper::GetRawJson(field.value, raw)) {
                    merged.Key(std::string(field.key));
                    merged.Raw(raw);
                }
            }
            merged.EndObject();
            writer.Raw(merged.TakeString());
            return;
        }

        writer.StartObject();
        writer.Key("type");
        switch (prop.kind) {
            case PropertyKind::String:
                writer.String("string");
                break;
            case PropertyKind::Number:
                writer.String("number");
                break;
            case PropertyKind::Integer:
                writer.String("integer");
                break;
            case PropertyKind::Boolean:
                writer.String("boolean");
                break;
            case PropertyKind::Array:
                writer.String("array");
                break;
            case PropertyKind::Enum:
                writer.String("string");
                break;
            case PropertyKind::Object:
                writer.String("object");
                break;
        }

        if (!prop.description.empty()) {
            writer.Key("description");
            writer.String(prop.description);
        }

        if (prop.kind == PropertyKind::Array) {
            writer.Key("items");
            writer.StartObject();
            writer.Key("type");
            writer.String(prop.itemType.empty() ? "string" : prop.itemType);
            writer.EndObject();
        }

        if (prop.kind == PropertyKind::Enum) {
            writer.Key("enum");
            writer.StartArray();
            for (const auto& value : prop.enumValues) {
                writer.String(value);
            }
            writer.EndArray();
        }

        writer.EndObject();
    }

    std::vector<Property> m_properties;
};

/**
 * @brief 提示参数构建器
 *
 * 用于构建 MCP 提示的参数定义
 */
class PromptArgumentBuilder {
public:
    /**
     * @brief 添加参数
     */
    PromptArgumentBuilder& addArgument(const std::string& name,
                                       const std::string& description,
                                       bool required = false) {
        PromptArgument arg;
        arg.name = name;
        arg.description = description;
        arg.required = required;
        m_arguments.push_back(std::move(arg));
        return *this;
    }

    /**
     * @brief 构建参数列表
     */
    std::vector<PromptArgument> build() const {
        return m_arguments;
    }

private:
    std::vector<PromptArgument> m_arguments;
};

} // namespace mcp
} // namespace galay
#endif // GALAY_MCP_COMMON_MCPSCHEMABUILDER_H
