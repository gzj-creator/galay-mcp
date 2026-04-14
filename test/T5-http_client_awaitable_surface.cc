/**
 * @file T5-http_client_awaitable_surface.cc
 * @brief 锁定 McpHttpClient 的 connect/disconnect 公开 awaitable surface 与底层 HttpClient 保持一致。
 */

#include "galay-mcp/client/McpHttpClient.h"

#include <concepts>
#include <string>
#include <utility>

using galay::http::HttpClient;
using galay::mcp::McpHttpClient;

static_assert(requires(McpHttpClient& client, const std::string& url) {
    {
        client.connect(url)
    } -> std::same_as<decltype(std::declval<HttpClient&>().connect(std::declval<const std::string&>()))>;
    {
        client.disconnect()
    } -> std::same_as<decltype(std::declval<HttpClient&>().close())>;
});

int main()
{
    return 0;
}
