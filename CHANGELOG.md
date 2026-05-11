# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `

## [Unreleased]

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
