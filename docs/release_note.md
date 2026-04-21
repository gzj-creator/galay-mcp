# Release Note

按时间顺序追加版本记录，避免覆盖历史发布说明。

## v1.1.2 - 2026-04-21

- 版本级别：小版本（patch）
- Git 提交消息：`chore: 发布 v1.1.2`
- Git Tag：`v1.1.2`
- 自述摘要：
  - 锁定 `galay-kernel 3.4.4` 与 `GalayHttp 2.0.2` 的依赖版本，确保 `galay-mcp` 在最新基础库前缀下稳定构建和安装。
  - 同步安装导出的 `galay-mcp-config.cmake` 依赖声明，减少下游回落到旧版本包的风险。
