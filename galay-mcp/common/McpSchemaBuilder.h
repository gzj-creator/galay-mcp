#ifndef GALAY_MCP_COMMON_MCPSCHEMABUILDER_H
#define GALAY_MCP_COMMON_MCPSCHEMABUILDER_H

#include "McpBase.h"
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
    SchemaBuilder() {
        m_schema["type"] = "object";
        m_schema["properties"] = Json::object();
    }

    /**
     * @brief 添加字符串属性
     * @param name 属性名
     * @param description 属性描述
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addString(const std::string& name,
                            const std::string& description,
                            bool required = false) {
        Json prop = Json::object();
        prop["type"] = "string";
        prop["description"] = description;
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 添加数字属性
     * @param name 属性名
     * @param description 属性描述
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addNumber(const std::string& name,
                            const std::string& description,
                            bool required = false) {
        Json prop = Json::object();
        prop["type"] = "number";
        prop["description"] = description;
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 添加整数属性
     * @param name 属性名
     * @param description 属性描述
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addInteger(const std::string& name,
                             const std::string& description,
                             bool required = false) {
        Json prop = Json::object();
        prop["type"] = "integer";
        prop["description"] = description;
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 添加布尔属性
     * @param name 属性名
     * @param description 属性描述
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addBoolean(const std::string& name,
                             const std::string& description,
                             bool required = false) {
        Json prop = Json::object();
        prop["type"] = "boolean";
        prop["description"] = description;
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 添加数组属性
     * @param name 属性名
     * @param description 属性描述
     * @param itemType 数组元素类型
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addArray(const std::string& name,
                           const std::string& description,
                           const std::string& itemType = "string",
                           bool required = false) {
        Json prop = Json::object();
        prop["type"] = "array";
        prop["description"] = description;
        prop["items"] = Json::object();
        prop["items"]["type"] = itemType;
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 添加对象属性
     * @param name 属性名
     * @param description 属性描述
     * @param objectSchema 对象的 Schema
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addObject(const std::string& name,
                            const std::string& description,
                            const Json& objectSchema,
                            bool required = false) {
        Json prop = objectSchema;
        prop["description"] = description;
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 添加枚举属性
     * @param name 属性名
     * @param description 属性描述
     * @param enumValues 枚举值列表
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    SchemaBuilder& addEnum(const std::string& name,
                          const std::string& description,
                          const std::vector<std::string>& enumValues,
                          bool required = false) {
        Json prop = Json::object();
        prop["type"] = "string";
        prop["description"] = description;
        prop["enum"] = Json::array();
        for (const auto& value : enumValues) {
            prop["enum"].push_back(value);
        }
        m_schema["properties"][name] = prop;

        if (required) {
            addRequired(name);
        }
        return *this;
    }

    /**
     * @brief 构建最终的 Schema
     * @return JSON Schema 对象
     */
    Json build() const {
        return m_schema;
    }

private:
    void addRequired(const std::string& name) {
        if (!m_schema.contains("required")) {
            m_schema["required"] = Json::array();
        }
        m_schema["required"].push_back(name);
    }

    Json m_schema;
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
     * @param name 参数名
     * @param description 参数描述
     * @param required 是否必需
     * @return 返回自身引用，支持链式调用
     */
    PromptArgumentBuilder& addArgument(const std::string& name,
                                       const std::string& description,
                                       bool required = false) {
        Json arg = Json::object();
        arg["name"] = name;
        arg["description"] = description;
        arg["required"] = required;
        m_arguments.push_back(arg);
        return *this;
    }

    /**
     * @brief 构建参数列表
     * @return 参数 JSON 数组
     */
    std::vector<Json> build() const {
        return m_arguments;
    }

private:
    std::vector<Json> m_arguments;
};

} // namespace mcp
} // namespace galay
#endif // GALAY_MCP_COMMON_MCPSCHEMABUILDER_H
