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
