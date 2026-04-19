use axum::{
    body::Bytes,
    http::StatusCode,
    response::{IntoResponse, Response},
    routing::{get, post},
    Json, Router,
};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::{
    env,
    net::{IpAddr, Ipv4Addr, SocketAddr},
    str::FromStr,
};
use tokio::signal;
use tracing::info;
use tracing_subscriber::{fmt, EnvFilter};

#[derive(Deserialize)]
struct JsonRpcRequest {
    jsonrpc: Option<String>,
    id: Option<Value>,
    method: Option<String>,
    params: Option<Value>,
}

#[derive(Serialize)]
struct JsonRpcResponse {
    jsonrpc: &'static str,
    id: Option<Value>,
    result: Value,
}

#[derive(Serialize)]
struct JsonRpcErrorBody {
    code: i64,
    message: String,
}

#[derive(Serialize)]
struct JsonRpcErrorResponse {
    jsonrpc: &'static str,
    id: Option<Value>,
    error: JsonRpcErrorBody,
}

fn error_response(id: Option<Value>, code: i64, message: impl Into<String>) -> Response {
    let response = JsonRpcErrorResponse {
        jsonrpc: "2.0",
        id,
        error: JsonRpcErrorBody {
            code,
            message: message.into(),
        },
    };
    (StatusCode::OK, Json(response)).into_response()
}

async fn handle(body: Bytes) -> Response {
    let payload: JsonRpcRequest = match serde_json::from_slice(&body) {
        Ok(payload) => payload,
        Err(err) => return error_response(None, -32700, format!("Parse error: {err}")),
    };

    if payload.jsonrpc.as_deref() != Some("2.0") {
        return error_response(payload.id, -32600, "Invalid Request".to_string());
    }

    info!(
        "Received method={} id={:?}",
        payload.method.as_deref().unwrap_or("<none>"),
        payload.id
    );

    let result = match payload.method.as_deref() {
        Some("initialize") => json!({
            "protocolVersion": "2024-11-05",
            "serverInfo": {
                "name": "galay-rust-benchmark",
                "version": "1.0.0",
                "capabilities": {
                    "tools": {},
                    "resources": {},
                    "prompts": {},
                    "logging": {}
                }
            },
            "capabilities": {
                "tools": {},
                "resources": {},
                "prompts": {},
                "logging": {}
            }
        }),
        Some("ping") => json!({}),
        Some("tools/list") => json!({
            "tools": [
                {
                    "name": "echo",
                    "description": "Echo back the input message",
                    "inputSchema": {
                        "type": "object",
                        "properties": {
                            "message": { "type": "string" }
                        },
                        "required": ["message"]
                    }
                },
                {
                    "name": "add",
                    "description": "Add two numbers",
                    "inputSchema": {
                        "type": "object",
                        "properties": {
                            "a": { "type": "number" },
                            "b": { "type": "number" }
                        },
                        "required": ["a", "b"]
                    }
                }
            ]
        }),
        Some("tools/call") => {
            let tool_name = payload
                .params
                .as_ref()
                .and_then(|params| params.get("name"))
                .and_then(Value::as_str)
                .unwrap_or_default();
            match tool_name {
                "echo" => {
                    let text = payload
                        .params
                        .as_ref()
                        .and_then(|params| params.get("arguments"))
                        .and_then(|args| args.get("message"))
                        .and_then(Value::as_str)
                        .unwrap_or_default();
                    json!({
                        "content": [
                            {
                                "type": "text",
                                "text": json!({
                                    "echo": text,
                                    "length": text.chars().count()
                                }).to_string()
                            }
                        ]
                    })
                }
                "add" => {
                    let args = payload
                        .params
                        .as_ref()
                        .and_then(|params| params.get("arguments"))
                        .cloned()
                        .unwrap_or_else(|| json!({}));
                    let a = args.get("a").and_then(Value::as_f64).unwrap_or(0.0);
                    let b = args.get("b").and_then(Value::as_f64).unwrap_or(0.0);
                    json!({
                        "content": [
                            {
                                "type": "text",
                                "text": json!({"sum": a + b}).to_string()
                            }
                        ]
                    })
                }
                _ => {
                    return error_response(payload.id, -32601, "Method not found".to_string());
                }
            }
        }
        Some("resources/list") => json!({
            "resources": [
                {
                    "uri": "example://hello",
                    "name": "Hello Resource",
                    "description": "A simple hello message",
                    "mimeType": "text/plain"
                },
                {
                    "uri": "example://info",
                    "name": "Info Resource",
                    "description": "Information about the server",
                    "mimeType": "text/plain"
                }
            ]
        }),
        Some("resources/read") => json!({
            "contents": [
                {
                    "type": "text",
                    "text": payload
                        .params
                        .as_ref()
                        .and_then(|params| params.get("uri"))
                        .and_then(Value::as_str)
                        .map(|uri| match uri {
                            "example://info" => "This is a test resource from the HTTP MCP server.",
                            _ => "Hello from MCP HTTP Server!",
                        })
                        .unwrap_or("Hello from MCP HTTP Server!"),
                    "uri": payload
                        .params
                        .as_ref()
                        .and_then(|params| params.get("uri"))
                        .and_then(Value::as_str)
                        .unwrap_or("example://hello"),
                    "mimeType": "text/plain"
                }
            ]
        }),
        Some("prompts/list") => json!({
            "prompts": [
                {
                    "name": "greeting",
                    "description": "Generate a friendly greeting",
                    "arguments": [
                        {
                            "name": "name",
                            "description": "User's name",
                            "required": false
                        }
                    ]
                }
            ]
        }),
        Some("prompts/get") => {
            let name = payload
                .params
                .as_ref()
                .and_then(|params| params.get("arguments"))
                .and_then(|args| args.get("name"))
                .and_then(Value::as_str)
                .unwrap_or("User");
            json!({
                "description": "A friendly greeting",
                "messages": [
                    {
                        "role": "user",
                        "content": {
                            "type": "text",
                            "text": format!("Hello, {name}! How can I help you today?")
                        }
                    }
                ]
            })
        }
        Some("notifications/initialized") => json!({}),
        Some(_) | None => {
            return error_response(payload.id, -32601, "Method not found".to_string())
        }
    };

    let response = JsonRpcResponse {
        jsonrpc: "2.0",
        id: payload.id,
        result,
    };

    (StatusCode::OK, Json(response)).into_response()
}

async fn health() -> impl IntoResponse {
    (StatusCode::OK, "ok")
}

async fn shutdown_signal() {
    signal::ctrl_c()
        .await
        .expect("failed to install Ctrl+C handler");
    info!("Received shutdown request.");
}

struct Config {
    host: IpAddr,
    port: u16,
}

impl Config {
    fn from_args() -> Self {
        let mut host = IpAddr::V4(Ipv4Addr::LOCALHOST);
        let mut port = 8081;
        let mut args = env::args().skip(1);

        while let Some(arg) = args.next() {
            match arg.as_str() {
                "--port" => {
                    if let Some(val) = args.next() {
                        port = val.parse().unwrap_or(port);
                    }
                }
                "--host" => {
                    if let Some(val) = args.next() {
                        if let Ok(parsed) = IpAddr::from_str(&val) {
                            host = parsed;
                        }
                    }
                }
                "--help" | "-h" => {
                    print_usage();
                    std::process::exit(0);
                }
                _ => {
                    eprintln!("Unknown argument: {}", arg);
                    print_usage();
                    std::process::exit(1);
                }
            }
        }

        Self { host, port }
    }
}

fn print_usage() {
    let program_name = env::args().next();
    let program = program_name.as_deref().unwrap_or("galay-rust-benchmark");
    println!(
        "Usage: {program} [--host <IP>] [--port <port>]\n  --host   IP address to bind (default: 127.0.0.1)\n  --port   TCP port to listen (default: 8081)\n  --help   Show this message",
        program = program
    );
}

#[tokio::main]
async fn main() {
    fmt()
        .with_env_filter(EnvFilter::from_default_env().add_directive("info".parse().unwrap()))
        .init();

    let config = Config::from_args();
    let addr = SocketAddr::new(config.host, config.port);

    info!("Rust baseline server listening on http://{}", addr);

    let app = Router::new()
        .route("/mcp", post(handle).get(health))
        .route("/health", get(health));

    let listener = tokio::net::TcpListener::bind(addr)
        .await
        .expect("listener bind failed");

    axum::serve(listener, app.into_make_service())
        .with_graceful_shutdown(shutdown_signal())
        .await
        .expect("server failed");
}
