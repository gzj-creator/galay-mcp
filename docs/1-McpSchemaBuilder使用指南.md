# McpSchemaBuilder 使用指南

## 概述

`McpSchemaBuilder` 提供了两个工具类来简化 MCP 服务器的配置：
- `SchemaBuilder`: 用于构建工具的 JSON Schema
- `PromptArgumentBuilder`: 用于构建提示的参数定义

## SchemaBuilder 使用示例

### 基本用法

```cpp
#include "galay-mcp/common/McpSchemaBuilder.h"

using namespace galay::mcp;

// 构建一个简单的 Schema
auto schema = SchemaBuilder()
    .addString("name", "User's name", true)  // 必需参数
    .addNumber("age", "User's age", false)   // 可选参数
    .build();
```

### 对比：使用前 vs 使用后

**使用前（繁琐）：**
```cpp
Json echoSchema;
echoSchema["type"] = "object";
echoSchema["properties"] = Json::object();
echoSchema["properties"]["message"] = Json::object();
echoSchema["properties"]["message"]["type"] = "string";
echoSchema["properties"]["message"]["description"] = "The message to echo";
echoSchema["required"] = Json::array({"message"});

server.addTool("echo", "Echo back the input message", echoSchema, echoTool);
```

**使用后（简洁）：**
```cpp
auto echoSchema = SchemaBuilder()
    .addString("message", "The message to echo", true)
    .build();

server.addTool("echo", "Echo back the input message", echoSchema, echoTool);
```

### 支持的数据类型

#### 1. 字符串类型
```cpp
auto schema = SchemaBuilder()
    .addString("username", "User's username", true)
    .build();
```

#### 2. 数字类型
```cpp
auto schema = SchemaBuilder()
    .addNumber("price", "Product price", true)
    .addNumber("discount", "Discount percentage", false)
    .build();
```

#### 3. 整数类型
```cpp
auto schema = SchemaBuilder()
    .addInteger("count", "Item count", true)
    .build();
```

#### 4. 布尔类型
```cpp
auto schema = SchemaBuilder()
    .addBoolean("enabled", "Whether feature is enabled", false)
    .build();
```

#### 5. 数组类型
```cpp
auto schema = SchemaBuilder()
    .addArray("tags", "List of tags", "string", false)
    .build();
```

#### 6. 枚举类型
```cpp
auto schema = SchemaBuilder()
    .addEnum("status", "Order status", {"pending", "processing", "completed"}, true)
    .build();
```

#### 7. 对象类型
```cpp
Json addressSchema = Json::object();
addressSchema["type"] = "object";
addressSchema["properties"] = Json::object();
addressSchema["properties"]["street"] = Json::object();
addressSchema["properties"]["street"]["type"] = "string";

auto schema = SchemaBuilder()
    .addString("name", "User's name", true)
    .addObject("address", "User's address", addressSchema, false)
    .build();
```

### 复杂示例

```cpp
// 创建一个复杂的工具 Schema
auto searchSchema = SchemaBuilder()
    .addString("query", "Search query", true)
    .addInteger("limit", "Maximum results", false)
    .addInteger("offset", "Result offset", false)
    .addArray("fields", "Fields to return", "string", false)
    .addEnum("sort", "Sort order", {"asc", "desc"}, false)
    .addBoolean("includeMetadata", "Include metadata", false)
    .build();

server.addTool("search", "Search for items", searchSchema, searchTool);
```

## PromptArgumentBuilder 使用示例

### 基本用法

```cpp
// 构建提示参数
auto promptArgs = PromptArgumentBuilder()
    .addArgument("name", "User's name", false)
    .addArgument("language", "Preferred language", false)
    .build();

server.addPrompt("greeting", "Generate a greeting", promptArgs, greetingPrompt);
```

### 对比：使用前 vs 使用后

**使用前（繁琐）：**
```cpp
Json promptArgs = Json::array();
Json nameArg;
nameArg["name"] = "name";
nameArg["description"] = "User's name";
nameArg["required"] = false;
promptArgs.push_back(nameArg);

Json langArg;
langArg["name"] = "language";
langArg["description"] = "Preferred language";
langArg["required"] = false;
promptArgs.push_back(langArg);

server.addPrompt("greeting", "Generate a greeting", promptArgs, greetingPrompt);
```

**使用后（简洁）：**
```cpp
auto promptArgs = PromptArgumentBuilder()
    .addArgument("name", "User's name", false)
    .addArgument("language", "Preferred language", false)
    .build();

server.addPrompt("greeting", "Generate a greeting", promptArgs, greetingPrompt);
```

## 完整示例

```cpp
#include "galay-mcp/server/McpHttpServer.h"
#include "galay-mcp/common/McpSchemaBuilder.h"

using namespace galay::mcp;

int main() {
    McpHttpServer server("0.0.0.0", 8080);
    server.setServerInfo("my-mcp-server", "1.0.0");

    // 添加计算器工具
    auto calcSchema = SchemaBuilder()
        .addEnum("operation", "Math operation", {"add", "subtract", "multiply", "divide"}, true)
        .addNumber("a", "First operand", true)
        .addNumber("b", "Second operand", true)
        .build();
    server.addTool("calculator", "Perform math operations", calcSchema, calculatorTool);

    // 添加搜索工具
    auto searchSchema = SchemaBuilder()
        .addString("query", "Search query", true)
        .addInteger("limit", "Max results", false)
        .addBoolean("fuzzy", "Enable fuzzy search", false)
        .build();
    server.addTool("search", "Search database", searchSchema, searchTool);

    // 添加用户信息提示
    auto userPromptArgs = PromptArgumentBuilder()
        .addArgument("userId", "User ID", true)
        .addArgument("includeHistory", "Include user history", false)
        .build();
    server.addPrompt("userInfo", "Get user information", userPromptArgs, userInfoPrompt);

    server.start();
    return 0;
}
```

## 优势

1. **代码简洁**：减少 70% 的样板代码
2. **类型安全**：编译时检查类型
3. **易于维护**：链式调用清晰易读
4. **减少错误**：避免手动构建 JSON 时的拼写错误
5. **自文档化**：代码即文档，一目了然

## API 参考

### SchemaBuilder

| 方法 | 参数 | 说明 |
|------|------|------|
| `addString()` | name, description, required | 添加字符串属性 |
| `addNumber()` | name, description, required | 添加数字属性 |
| `addInteger()` | name, description, required | 添加整数属性 |
| `addBoolean()` | name, description, required | 添加布尔属性 |
| `addArray()` | name, description, itemType, required | 添加数组属性 |
| `addObject()` | name, description, objectSchema, required | 添加对象属性 |
| `addEnum()` | name, description, enumValues, required | 添加枚举属性 |
| `build()` | - | 构建最终的 Schema |

### PromptArgumentBuilder

| 方法 | 参数 | 说明 |
|------|------|------|
| `addArgument()` | name, description, required | 添加参数 |
| `build()` | - | 构建参数列表 |
