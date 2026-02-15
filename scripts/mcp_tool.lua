-- mcp_tool.lua
wrk.method = "POST"
wrk.body = '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"echo","arguments":{"message":"wrk"}}}'
wrk.headers["Content-Type"] = "application/json"
