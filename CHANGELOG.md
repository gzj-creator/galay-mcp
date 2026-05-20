# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `

## [Unreleased]

## [v2.1.0] - 2026-05-20

### Added
- 新增 `galay::mcp::log::set/get` 库级日志入口，使用 `galay-kernel` 的 `BaseLogger` 与独立 logger 槽位。
- 新增 `MCP_LOG_*` 埋点宏，并在 stdio/http client 与 server 的连接、协议错误和读写失败路径补充日志。
- 新增 `t7_log` 回归测试，验证未设置 logger 和级别过滤时不会求值日志格式化参数。

### Changed
- 将 `galay-http` 依赖提升到 `3.1.0`，继续对齐 `galay-kernel 5.0.0`。
- 为主库目标启用 `NO_SYSTEM_FROM_IMPORTED`，避免本机旧 `/usr/local/include` 中的 Galway 包头文件抢先命中。
- 修正测试和 benchmark 脚本中的可执行文件名，使其匹配小写蛇形测试目标。
- 将 CMake project/package 版本提升到 `2.1.0`。

## [v2.0.2] - 2026-05-18

### Changed
- 将安装导出的 CMake targets 文件改为 `galayMcpConfigTargets.cmake`，同步 package config 的 include 路径。
- Release 安装现在生成 `galayMcpConfigTargets-release.cmake`，与新的驼峰导出文件命名保持一致。
- 将 CMake project 版本提升到 `2.0.2`，对齐本次发布 tag。


## [v2.0.1] - 2026-05-11

### Chore
- 移除 `benchmark/compare` 目录，避免误提交对比基准测试代码与构建产物。

## [v2.0.0] - 2026-04-29

### Changed
- 统一源码、头文件、测试、示例与 benchmark 文件命名为 `lower_snake_case`，编号前缀同步使用 `t<number>_`、`e<number>_` 与 `b<number>_` 风格。
- 同步更新构建脚本、模块入口、示例、测试、文档与脚本中的文件路径引用。
- 将项目内头文件包含调整为基于公开 include 根或模块根的非相对路径。

### Release
- 按大版本发布要求提升版本到 `v2.0.0`。

## [v1.1.3] - 2026-04-23

### Changed
- 将 `BUILD_TESTS` 收敛为 `BUILD_TESTING` 的兼容别名，未显式设置时默认关闭测试构建。
- 保留对旧别名的 deprecation 提示，显式使用兼容开关时仍能恢复测试目标。

## [v1.1.2] - 2026-04-21

### Changed
- 锁定 `galay-kernel 3.4.4` 与 `GalayHttp 2.0.2` 的 CMake 依赖版本，避免误链接旧前缀中的基础库。
- 同步导出包配置中的 `find_dependency(...)` 版本约束，确保源码构建、测试与下游消费使用同一组内部依赖版本。
