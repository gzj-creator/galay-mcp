# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `## [Unreleased]`。
- 需要发版时，从 `Unreleased` 或“自上次 tag 以来”的累计变更整理出新的版本节。
- 版本号遵循 `major/minor/patch` 规则：大改动升主版本，新功能升次版本，修复与非破坏性维护升修订版本。
- 推荐标题格式为 `## [vX.Y.Z] - YYYY-MM-DD`，正文按 `Added` / `Changed` / `Fixed` / `Docs` / `Chore` 归纳。

## [Unreleased]

### Changed
- 将 `galay-http` 依赖消费入口切换为 `find_package(galay-http 2.0.2 CONFIG REQUIRED)` 与 `galay-http::galay-http`，匹配 HTTP 包的小写导出风格。

## [v1.1.3] - 2026-04-23

### Changed
- 将 `BUILD_TESTS` 收敛为 `BUILD_TESTING` 的兼容别名，未显式设置时默认关闭测试构建。
- 保留对旧别名的 deprecation 提示，显式使用兼容开关时仍能恢复测试目标。

## [v1.1.2] - 2026-04-21

### Changed
- 锁定 `galay-kernel 3.4.4` 与 `GalayHttp 2.0.2` 的 CMake 依赖版本，避免误链接旧前缀中的基础库。
- 同步导出包配置中的 `find_dependency(...)` 版本约束，确保源码构建、测试与下游消费使用同一组内部依赖版本。
