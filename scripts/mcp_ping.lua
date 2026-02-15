-- mcp_ping.lua
wrk.method = "POST"
wrk.body = '{"jsonrpc":"2.0","id":1,"method":"ping","params":{}}'
wrk.headers["Content-Type"] = "application/json"
