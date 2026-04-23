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
