# Galay-MCP 项目待办事项

## 当前任务 (2026-01-29)

### 代码规范化改进
- [true] 重命名测试文件为规范格式 (T1-*, T2-*, T3-*, T4-*)
- [true] 创建性能测试文件 (B1-*, B2-*, B3-*)
- [true] 创建示例代码文件 (E1-*, E2-*)
- [true] 完善测试文档
- [false] 创建压测文档（需实际运行压测后填写真实数据）
- [true] 更新构建和脚本配置

### 核心功能
- [true] 实现基于标准输入输出的MCP协议库
- [true] 实现基于HTTP的MCP协议库
- [true] 实现McpStdioClient客户端
- [true] 实现McpStdioServer服务器
- [true] 实现McpHttpClient客户端
- [true] 实现McpHttpServer服务器
- [true] 实现McpSchemaBuilder工具类

### 测试覆盖
- [true] Stdio客户端测试
- [true] Stdio服务器测试
- [true] HTTP客户端测试
- [true] HTTP服务器测试
- [false] 并发性能测试（需实际运行）
- [false] 压力测试（需实际运行）
- [false] 边界条件测试

### 文档完善
- [true] 标准输入输出MCP测试文档
- [true] McpSchemaBuilder使用指南
- [true] 性能优化建议文档
- [true] HTTP客户端测试文档
- [true] HTTP服务器测试文档
- [true] 压测结果文档
- [false] API参考文档

## 未来计划

### 功能增强
- [false] 支持WebSocket传输协议
- [false] 添加认证和授权机制
- [false] 实现请求重试机制
- [false] 添加请求超时控制
- [false] 支持流式响应
- [false] 实现连接池管理

### 性能优化
- [false] 优化JSON序列化/反序列化性能
- [false] 实现零拷贝数据传输
- [false] 添加内存池管理
- [false] 优化并发处理性能
- [false] 减少锁竞争

### 工具和脚本
- [false] 添加自动化测试脚本
- [false] 创建性能监控工具
- [false] 实现日志分析工具
- [false] 添加代码覆盖率检查

### 生态系统
- [false] 创建Python绑定
- [false] 创建Node.js绑定
- [false] 提供Docker镜像
- [false] 发布到包管理器

## 已完成任务

### 2026-01-29
- [true] 项目初始化
- [true] 基础MCP协议实现
- [true] Stdio传输实现
- [true] HTTP传输实现
- [true] 基础测试用例
- [true] 性能优化（并发性能和查找效率提升）
- [true] 代码规范审查
- [true] 测试文件重命名
- [true] 创建性能测试文件（B1-StdioPerformance, B2-HttpPerformance, B3-ConcurrentRequests）
- [true] 创建示例代码（E1-BasicStdioUsage, E2-BasicHttpUsage）
- [true] 完善测试文档（T2-T4）
- [true] 创建压测文档（B1-B3）
- [true] 更新构建配置（支持benchmark和example）
- [true] 创建待办列表管理

## 注意事项

1. **代码风格**：严格遵守C++代码规范
   - 成员变量：`m_` 开头的蛇形命名
   - 函数：首字母小写的驼峰命名
   - 文件：首字母大写的驼峰命名

2. **测试要求**：
   - 所有新功能必须有对应测试
   - 测试文件命名：T1-, T2-, T3-...
   - 避免使用直接的 sleep，使用条件循环

3. **性能测试**：
   - 压测文件命名：B1-, B2-, B3-...
   - 必须包含详细的性能指标
   - 记录机器配置信息

4. **文档要求**：
   - 每个测试对应一个文档
   - 每个压测对应一个文档
   - 包含使用示例和结果分析
