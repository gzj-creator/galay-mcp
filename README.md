# Galay MCP

`galay-mcp` 是一个基于 C++23 的 MCP（Model Context Protocol）实现，仓库同时提供：

- `stdio` 传输的客户端与服务器
- `HTTP` 传输的客户端与服务器
- `examples/` 中的 include / import 示例
- `test/` 中的可执行测试程序
- `benchmark/` 中的性能基准程序

## 当前状态

- 已实现：`McpStdioServer`、`McpStdioClient`、`McpHttpServer`、`McpHttpClient`
- 已实现：JSON Schema 构建器、资源与提示注册、JSON-RPC 2.0 / MCP 基础方法
- 条件支持：C++23 模块示例（`galay-mcp-modules`、`import galay.mcp;`）
- 未实现：WebSocket 传输

## 文档导航

- [文档总览](docs/README.md) - 文档阅读顺序与定位说明
- [00-快速开始](docs/00-快速开始.md) - 依赖、构建、首次运行
- [01-架构设计](docs/01-架构设计.md) - 分层设计与核心数据结构
- [02-API参考](docs/02-API参考.md) - 公开头文件、API 签名、前置条件、失败语义与示例 / 测试锚点
- [03-使用指南](docs/03-使用指南.md) - 使用模式与实践建议
- [04-示例代码](docs/04-示例代码.md) - 与真实 `examples/` / `test/` 对齐的示例索引
- [05-性能测试](docs/05-性能测试.md) - 基准目标、命令、验证方式与状态说明
- [06-高级主题](docs/06-高级主题.md) - 模块构建、运行时与扩展边界
- [07-常见问题](docs/07-常见问题.md) - 当前限制、排障与依赖说明

文档导航收敛到 `README.md` + `docs/README.md` + `docs/00`~`docs/07`。此前独立拆分的测试页已并回：

- [00-快速开始](docs/00-快速开始.md) - 首次联调与最短验证路径
- [04-示例代码](docs/04-示例代码.md) - `examples/` / `test/` / `scripts/` 的统一示例与回归入口
- [07-常见问题](docs/07-常见问题.md) - FIFO / transport / 排障说明

## 文档真相来源

当文档与仓库内容冲突时，以以下顺序为准：

1. `galay-mcp/` 下公开头文件与导出 target
2. 对应 `.cc` 实现行为
3. `examples/`
4. `test/`
5. `benchmark/`
6. Markdown 文档

## 仓库结构

```text
.
├── CMakeLists.txt
├── galay-mcp/
│   ├── client/
│   │   ├── McpStdioClient.h
│   │   └── McpHttpClient.h
│   ├── server/
│   │   ├── McpStdioServer.h
│   │   └── McpHttpServer.h
│   ├── common/
│   │   ├── McpBase.h
│   │   ├── McpError.h
│   │   ├── McpJson.h
│   │   └── McpSchemaBuilder.h
│   └── module/
│       └── galay.mcp.cppm
├── examples/
│   ├── include/
│   │   ├── E1-basic_stdio_usage.cc
│   │   └── E2-basic_http_usage.cc
│   ├── import/
│   │   ├── E1-basic_stdio_usage.cc
│   │   └── E2-basic_http_usage.cc
│   └── common/
├── test/
│   ├── T1-stdio_client.cc
│   ├── T2-stdio_server.cc
│   ├── T3-http_client.cc
│   └── T4-http_server.cc
├── benchmark/
│   ├── B1-stdio_performance.cc
│   ├── B2-http_performance.cc
│   └── B3-concurrent_requests.cc
├── docs/
└── scripts/
```

## 依赖与构建前提

### 版本要求

- CMake：`>= 3.20`
- C++：`CMAKE_CXX_STANDARD 23`
- 模块示例：`CMake >= 3.28`，且生成器需支持模块扫描（`Ninja` 或 `Visual Studio`）
- 使用 Clang 构建模块示例时，还需要 `clang-scan-deps`

### 外部依赖矩阵

| 场景 | 必需外部依赖 | 原因 |
| --- | --- | --- |
| 默认库构建 `galay-mcp` | `simdjson`、`galay-kernel`、`galay-http` | 当前 `galay-mcp/CMakeLists.txt` 会始终编译 `client/*.cc` 与 `server/*.cc`，HTTP 公开头文件无条件包含 `galay-http` / `galay-kernel` 头 |
| `stdio` 示例 / 测试 | 同上 | 这些 target 统一链接到默认库 target，当前仓库没有单独的 `stdio-only` 库开关 |
| HTTP 示例 / 测试 / benchmark | 同上 | 额外依赖 HTTP 运行时与网络栈 |
| C++23 模块示例 | 同上 + 模块工具链支持 | 需要生成 `galay-mcp-modules` target |

> 结论：当前仓库的“默认构建”不是 `simdjson-only`。如果缺少 `galay-kernel` 或 `galay-http`，请先补齐依赖，再执行 CMake 配置。

### 安装示例

```bash
# macOS (Homebrew)
brew install cmake simdjson

# Ubuntu / Debian
sudo apt-get update
sudo apt-get install -y cmake g++ libsimdjson-dev
```

`galay-kernel` 与 `galay-http` 当前需要按各自仓库的说明完成安装，并保证头文件 / 库可被 CMake 找到。

## 构建

```bash
cmake -S . -B build -DBUILD_MODULE_EXAMPLES=OFF
cmake --build build
```

常用选项：

```bash
# 关闭测试 / CTest 注册
cmake -S . -B build -DBUILD_TESTING=OFF

# 关闭 benchmark
cmake -S . -B build -DBUILD_BENCHMARKS=OFF

# 关闭 examples
cmake -S . -B build -DBUILD_EXAMPLES=OFF

# 尝试开启模块示例（仅在支持环境下）
cmake -S . -B build -DBUILD_MODULE_EXAMPLES=ON -G Ninja
```

`BUILD_TESTS` 仍保留为旧脚本兼容选项；新的 CI / CTest 入口请优先使用 `BUILD_TESTING`。

## 快速验证

### 1. Stdio 原始协议验证

直接向 `T2-stdio_server` 输入 JSON-RPC 请求即可验证协议面：

```bash
printf '%s\n' \
  '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"readme-check","version":"1.0.0"},"capabilities":{}}}' \
  | ./build/bin/T2-stdio_server
```

### 2. Stdio 双向客户端 / 服务器联调

`stdio` 是双向流，单个 shell pipe 不足以完成完整会话。请使用两条 FIFO：

```bash
mkfifo /tmp/galay-mcp-c2s /tmp/galay-mcp-s2c
./build/bin/T2-stdio_server < /tmp/galay-mcp-c2s > /tmp/galay-mcp-s2c &
SERVER_PID=$!
./build/bin/T1-stdio_client > /tmp/galay-mcp-c2s < /tmp/galay-mcp-s2c
kill ${SERVER_PID}
rm -f /tmp/galay-mcp-c2s /tmp/galay-mcp-s2c
```

仓库内也提供了同等思路的脚本：`scripts/S4-RunIntegrationTest.sh`。

### 3. HTTP 联调

终端 1：

```bash
./build/bin/T4-http_server 8080 0.0.0.0
```

终端 2：

```bash
./build/bin/T3-http_client http://127.0.0.1:8080/mcp
```

## 公开 API 与模块名

- 核心 target：`galay-mcp`
- 模块 target：`galay-mcp-modules`（条件生成）
- 模块名：`galay.mcp`
- 主要公开头文件：
  - `galay-mcp/common/McpBase.h`
  - `galay-mcp/common/McpError.h`
  - `galay-mcp/common/McpJson.h`
  - `galay-mcp/common/McpJsonParser.h`
  - `galay-mcp/common/McpProtocolUtils.h`
  - `galay-mcp/common/McpSchemaBuilder.h`
  - `galay-mcp/client/McpStdioClient.h`
  - `galay-mcp/client/McpHttpClient.h`
  - `galay-mcp/server/McpStdioServer.h`
  - `galay-mcp/server/McpHttpServer.h`
  - `galay-mcp/module/ModulePrelude.hpp`（模块构建兼容前导头）
  - `galay-mcp/module/galay.mcp.cppm`（模块接口文件）

详细签名与每个入口的前置条件 / 失败路径 / 示例锚点见 [02-API参考](docs/02-API参考.md)。

## 示例、测试与基准索引

### 示例

| 来源文件 | target | 运行命令 | 必需环境变量 |
| --- | --- | --- | --- |
| `examples/include/E1-basic_stdio_usage.cc` | `E1-BasicStdioUsage` | `./build/bin/E1-BasicStdioUsage server` / `client` | 无 |
| `examples/import/E1-basic_stdio_usage.cc` | `E1-BasicStdioUsageImport` | 同上（仅模块构建成功时存在） | 无 |
| `examples/include/E2-basic_http_usage.cc` | `E2-BasicHttpUsage` | `./build/bin/E2-BasicHttpUsage server` / `client http://127.0.0.1:8080/mcp` | 无 |
| `examples/import/E2-basic_http_usage.cc` | `E2-BasicHttpUsageImport` | 同上（仅模块构建成功时存在） | 无 |

### 测试

| 来源文件 | target | 说明 |
| --- | --- | --- |
| `test/T1-stdio_client.cc` | `T1-stdio_client` | stdio 客户端回归测试 |
| `test/T2-stdio_server.cc` | `T2-stdio_server` | stdio 服务端回归测试 |
| `test/T3-http_client.cc` | `T3-http_client` | HTTP 客户端回归测试 |
| `test/T4-http_server.cc` | `T4-http_server` | HTTP 服务端回归测试 |

### Benchmark

| 来源文件 | target | 状态 |
| --- | --- | --- |
| `benchmark/B1-stdio_performance.cc` | `B1-stdio_performance` | 当前文档仅保留真实命令与参数，未附带本次整改新跑出的数字 |
| `benchmark/B2-http_performance.cc` | `B2-http_performance` | 同上 |
| `benchmark/B3-concurrent_requests.cc` | `B3-concurrent_requests` | 同上 |

## 已知限制

- 当前默认构建没有 `stdio-only` 依赖裁剪开关。
- WebSocket 传输未实现；任何相关内容都应视为扩展思路，而不是现成能力。
- `stdio` 双向会话需要双向管道（FIFO / pty / 自定义 transport）；不要把单个 shell pipe 当作完整联调方案。
- `docs/05-性能测试.md` 当前不包含与本次提交绑定的延迟 / QPS 断言，只保留可复现命令与验证方式。

## 许可证

项目许可证见 `LICENSE`。
