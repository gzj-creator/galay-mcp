# Galay MCP

基于 Galay-Kernel 框架实现的 MCP (Model Context Protocol) 协议库。

## 📁 项目结构

```
galay-mcp/
├── CMakeLists.txt              # 根项目配置
├── README.md                   # 项目说明（本文件）
├── galay-mcp/                  # 核心库
│   ├── CMakeLists.txt          # 库构建配置
│   ├── common/                 # 通用模块
│   │   ├── McpBase.h           # 基础数据结构
│   │   ├── McpError.h          # 错误处理
│   │   └── McpError.cc         # 错误处理实现
│   ├── client/                 # 客户端实现
│   │   ├── McpStdioClient.h    # 标准输入输出客户端
│   │   └── McpStdioClient.cc   # 标准输入输出客户端实现
│   └── server/                 # 服务器实现
│       ├── McpStdioServer.h    # 标准输入输出服务器
│       └── McpStdioServer.cc   # 标准输入输出服务器实现
├── test/                       # 测试和示例
│   ├── CMakeLists.txt
│   ├── test_stdio_server.cc    # 服务器示例
│   └── test_stdio_client.cc    # 客户端示例
├── benchmark/                  # 性能测试
├── docs/                       # 文档
├── scripts/                    # 脚本
│   ├── run.sh                  # 运行脚本
│   └── check.sh                # 检查脚本
└── todo/                       # 待办事项
```

## ✨ 特性

- ✅ **标准输入输出**：支持基于 stdin/stdout 的 MCP 通信
- ✅ **简洁易用**：提供简洁的 API 接口
- ✅ **类型安全**：使用 C++23 和 std::expected 进行错误处理
- ✅ **标准兼容**：遵循 MCP 2024-11-05 规范
- ✅ **高性能**：基于 Galay-Kernel 框架的高效实现
- 🚧 **HTTP 传输**：计划支持（待实现）

## 📦 依赖

- C++23 编译器（GCC 13+, Clang 16+）
- [Galay-Kernel](https://github.com/GaiaKernel/galay) 框架
- [nlohmann/json](https://github.com/nlohmann/json) JSON 库

## 🔧 构建

### 前置要求

确保已经安装了所有依赖：

```bash
# 1. 安装 Galay-Kernel 框架（参考 Galay 项目的安装说明）
# 2. 安装 nlohmann/json

# macOS (使用 Homebrew)
brew install nlohmann-json

# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# 或者手动安装 header-only 版本到 /usr/local/include
cd /usr/local/include
sudo mkdir -p nlohmann
sudo curl -o nlohmann/json.hpp https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
```

### 编译步骤

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置 CMake
cmake ..

# 3. 编译
make -j4

# 4. （可选）安装到系统
sudo make install
```

### 构建选项

```bash
# 不构建测试
cmake -DBUILD_TESTS=OFF ..

# 不构建性能测试
cmake -DBUILD_BENCHMARK=OFF ..

# 安装到系统
cmake --build . --target install
```

## 🚀 快速开始

### 服务器端（标准输入输出）

```cpp
#include "galay-mcp/server/McpStdioServer.h"

McpStdioServer server;

// 添加工具
server.addTool("add", "Add two numbers",
    [](const nlohmann::json& args) -> std::expected<nlohmann::json, McpError> {
        int a = args["a"];
        int b = args["b"];
        return nlohmann::json{{"result", a + b}};
    }
);

// 启动服务器（从 stdin 读取，向 stdout 写入）
server.run();
```

### 客户端（标准输入输出）

```cpp
#include "galay-mcp/client/McpStdioClient.h"

McpStdioClient client;

// 初始化连接
auto init_result = client.initialize("MyClient", "1.0.0");
if (!init_result) {
    // 处理错误
}

// 调用工具
nlohmann::json args = {{"a", 10}, {"b", 20}};
auto result = client.callTool("add", args);
if (result) {
    std::cout << "Result: " << result.value() << std::endl;
}
```

## 🧪 运行测试

```bash
cd build

# 运行单元测试
./bin/test_stdio_server
./bin/test_stdio_client

# 运行集成测试（通过管道连接）
./bin/test_stdio_server | ./bin/test_stdio_client
```

## 📖 协议格式

### 标准输入输出协议

- **传输**: stdin/stdout
- **格式**: JSON-RPC 2.0
- **分隔**: 每条消息一行（换行符分隔）

### 请求示例

```json
{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"add","arguments":{"a":10,"b":20}}}
```

### 响应示例

```json
{"jsonrpc":"2.0","id":1,"result":{"content":[{"type":"text","text":"30"}]}}
```

## 📚 API 文档

### McpStdioServer

```cpp
class McpStdioServer {
public:
    // 添加工具
    void addTool(const std::string& name,
                 const std::string& description,
                 ToolHandler handler);

    // 添加资源
    void addResource(const std::string& uri,
                     const std::string& name,
                     const std::string& mimeType);

    // 添加提示
    void addPrompt(const std::string& name,
                   const std::string& description);

    // 运行服务器（阻塞）
    void run();

    // 停止服务器
    void stop();
};
```

### McpStdioClient

```cpp
class McpStdioClient {
public:
    // 初始化连接
    std::expected<void, McpError> initialize(
        const std::string& clientName,
        const std::string& clientVersion);

    // 调用工具
    std::expected<nlohmann::json, McpError> callTool(
        const std::string& toolName,
        const nlohmann::json& arguments);

    // 获取工具列表
    std::expected<std::vector<std::string>, McpError> listTools();

    // 获取资源列表
    std::expected<std::vector<std::string>, McpError> listResources();

    // 断开连接
    void disconnect();
};
```

## 🏗️ 架构设计

```
应用层：McpStdioClient/Server（标准输入输出实现）
    ↓
协议层：MCP JSON-RPC 2.0 消息处理
    ↓
编解码：nlohmann::json（JSON 序列化）
    ↓
传输层：stdin/stdout（标准输入输出流）
```

## 🔍 示例代码

完整示例请查看：
- [test/test_stdio_server.cc](test/test_stdio_server.cc) - 服务器示例
- [test/test_stdio_client.cc](test/test_stdio_client.cc) - 客户端示例

## 🛣️ 开发路线图

- [x] 标准输入输出传输层实现
- [x] 基础 MCP 协议支持
- [x] 简化 API 设计
- [ ] 完整的单元测试
- [ ] 性能测试和优化
- [ ] HTTP 传输支持（基于 Galay-Kernel）
- [ ] WebSocket 传输支持
- [ ] 文档完善

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 🙏 致谢

本项目基于以下优秀开源项目：
- [Galay-Kernel](https://github.com/GaiaKernel/galay) - 高性能 C++ 框架
- [nlohmann/json](https://github.com/nlohmann/json) - JSON 库
- [MCP](https://modelcontextprotocol.io/) - Model Context Protocol 规范
