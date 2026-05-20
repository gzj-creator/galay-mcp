# Release Note

按时间顺序追加版本记录，避免覆盖历史发布说明。

## v1.1.3 - 2026-04-23

- 版本级别：小版本（patch）
- Git 提交消息：`chore: 默认关闭测试构建`
- Git Tag：`v1.1.3`
- 自述摘要：
  - 将 `BUILD_TESTING` 固化为 `galay-mcp` 的测试主开关，未显式开启时默认强制关闭测试构建，避免根配置阶段被兼容选项隐式打开测试树。
  - 保留 `BUILD_TESTS` 兼容别名，并在通过旧参数开启测试时继续输出 deprecation warning，方便旧脚本平滑迁移。
  - 继续保留 `include(CTest)` 流程与现有测试目录门控，显式开启测试时仍可暴露 `test` 相关目标。

## v1.1.2 - 2026-04-21

- 版本级别：小版本（patch）
- Git 提交消息：`chore: 发布 v1.1.2`
- Git Tag：`v1.1.2`
- 自述摘要：
  - 锁定 `galay-kernel 3.4.4` 与 `GalayHttp 2.0.2` 的依赖版本，确保 `galay-mcp` 在最新基础库前缀下稳定构建和安装。
  - 同步安装导出的 `galay-mcp-config.cmake` 依赖声明，减少下游回落到旧版本包的风险。

## v2.0.0 - 2026-04-29

- 版本级别：大版本（major）
- Git 提交消息：`refactor: 统一源码文件命名规范`
- Git Tag：`v2.0.0`
- 自述摘要：
  - 将源码、头文件、测试、示例与 benchmark 文件统一重命名为 lower_snake_case，编号前缀同步改为小写下划线形式。
  - 同步更新 CMake/Bazel 构建描述、模块入口、README/docs、脚本和所有项目内 include 路径引用。
  - 移除项目内相对 include，统一使用基于公开 include 根或模块根的非相对路径。

## v2.0.1 - 2026-05-11

- 版本级别：小版本（patch）
- Git 提交消息：`chore: 移除 benchmark compare 目录`
- Git Tag：`v2.0.1`
- 自述摘要：
  - 移除 `benchmark/compare` 目录并收紧忽略规则，避免误提交对比基准测试代码与构建产物。

## v2.0.2 - 2026-05-18

- 版本级别：小版本（patch）
- Git 提交消息：`chore: 统一 CMake 导出文件命名`
- Git Tag：`v2.0.2`
- 自述摘要：
  - 将安装导出的 CMake targets 文件改为 `galayMcpConfigTargets.cmake`，并同步 `galay-mcp-config.cmake` 的 include 路径。
  - Release 安装随主 targets 文件生成 `galayMcpConfigTargets-release.cmake`，统一驼峰导出文件命名。
  - 将 CMake project 版本提升到 `2.0.2`，确保源码版本元数据、tag 与发布记录一致。

## v2.1.0 - 2026-05-20

- 版本级别：中版本（minor）
- Git 提交消息：`feat: 增加 mcp 库级 BaseLogger 日志入口`
- Git Tag：`v2.1.0`
- 自述摘要：
  - 新增 `galay::mcp::log::set/get` 库级日志入口，使用 `galay-kernel` 的 `BaseLogger` 与独立 logger 槽位，允许用户只启用 galay-mcp 日志。
  - 新增 `MCP_LOG_*` 埋点宏，并在 stdio/http client 与 server 的连接、协议错误和读写失败路径补充日志。
  - 新增 `t7_log` 回归测试，验证未设置 logger 和日志级别过滤时不会求值格式化参数。
  - 将 `galay-http` 依赖提升到 `3.1.0`，继续对齐 `galay-kernel 5.0.0`，并同步 CMake project/package 版本到 `2.1.0`。
  - 为主库目标启用 `NO_SYSTEM_FROM_IMPORTED`，避免本机旧 `/usr/local/include` 中的 Galway 包头文件抢先命中。
  - 修正测试和 benchmark 脚本中的可执行文件名，使其匹配小写蛇形测试目标。
