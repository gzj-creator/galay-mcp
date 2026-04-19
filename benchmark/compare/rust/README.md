# galay-mcp Rust benchmark compare scaffold

当前对标边界：

- `B1-stdio_performance`
  - 目前没有公平的主流 Rust stdio MCP 基线
  - 结论默认 `internal-only`
- `B2-http_performance`
  - 推荐 Rust 基线：`axum` / `hyper` / `tokio`
- `B3-concurrent_requests`
  - 推荐 Rust 基线：`axum` / `hyper` / `tokio`

对外发布要求：

1. 同机
2. 同构建类型
3. 同 workload
4. 同时保存命令、环境和原始 stdout

没有 Rust 基线的 workload 只能作为内部样本或历史样本解释。
