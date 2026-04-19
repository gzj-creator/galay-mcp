#!/usr/bin/env bash

# Rust 比较入口（Axum/Hyper/Tokio baseline + 现有 B2/B3 C++ workloads）

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
RUST_DIR="$REPO_ROOT/benchmark/compare/rust"
RUST_BIN_NAME="galay-rust-benchmark"
RUST_PORT="${RUST_PORT:-8081}"
SERVER_LOG="/tmp/galay-rust-baseline.log"
CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-$RUST_DIR/target}"
HOST_CARGO_HOME="${HOME:-}/.cargo"
CARGO_HOME="${CARGO_HOME:-}"
RUSTC_BIN="${RUSTC_BIN:-$(command -v rustc || true)}"
CARGO_BIN="${CARGO_BIN:-$(command -v cargo || true)}"
CURL_BIN="${CURL_BIN:-$(command -v curl || true)}"

echo "=== Rust Baseline Compare ==="
echo ""

if [ -z "$CARGO_BIN" ]; then
    echo "✗ cargo not found in PATH; install Rust toolchain to run the Rust baseline."
    exit 1
fi

if [ -z "$RUSTC_BIN" ]; then
    echo "✗ rustc not found in PATH; install Rust toolchain to run the Rust baseline."
    exit 1
fi

if [ -z "$CURL_BIN" ]; then
    echo "✗ curl not found; required for readiness checks."
    exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo "✗ C++ build directory '$BUILD_DIR' missing. Run cmake/cmake --build before starting benchmarks."
    exit 1
fi

if [ ! -d "$RUST_DIR" ]; then
    echo "✗ Rust compare directory '$RUST_DIR' missing."
    exit 1
fi

if [ -z "$CARGO_HOME" ]; then
    if [ -d "$HOST_CARGO_HOME" ] && [ -w "$HOST_CARGO_HOME" ]; then
        CARGO_HOME="$HOST_CARGO_HOME"
    else
        CARGO_HOME="${TMPDIR:-/tmp}/galay-mcp-cargo-home"
    fi
fi

mkdir -p "$CARGO_HOME" "$CARGO_TARGET_DIR"

if [ "$CARGO_HOME" != "$HOST_CARGO_HOME" ] && [ -f "$HOST_CARGO_HOME/config.toml" ] && [ ! -f "$CARGO_HOME/config.toml" ]; then
    cp "$HOST_CARGO_HOME/config.toml" "$CARGO_HOME/config.toml"
fi

echo "> Detected build dir: $BUILD_DIR"
echo "> Rust compare workspace: $RUST_DIR"
echo "> Cargo home: $CARGO_HOME"
echo "> Cargo target dir: $CARGO_TARGET_DIR"
echo "> rustc: $RUSTC_BIN"

pushd "$RUST_DIR" >/dev/null
echo "Building Rust baseline (release)..."
RUSTC="$RUSTC_BIN" CARGO_HOME="$CARGO_HOME" CARGO_TARGET_DIR="$CARGO_TARGET_DIR" "$CARGO_BIN" build --locked --release
popd >/dev/null

RUST_BIN="$CARGO_TARGET_DIR/release/$RUST_BIN_NAME"
if [ ! -x "$RUST_BIN" ]; then
    echo "✗ Rust benchmark binary '$RUST_BIN' not found after build."
    exit 1
fi

cleanup() {
    if [ -n "${SERVER_PID-}" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Stopping Rust baseline server..."
        kill "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo "Starting Rust baseline server on port $RUST_PORT..."
"$RUST_BIN" --port "$RUST_PORT" > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!

for attempt in {1..15}; do
    if "$CURL_BIN" -sS -o /dev/null "http://127.0.0.1:${RUST_PORT}/mcp"; then
        break
    fi
    sleep 0.3
done

if ! "$CURL_BIN" -sS -o /dev/null "http://127.0.0.1:${RUST_PORT}/mcp"; then
    echo "✗ Rust baseline server did not respond on http://127.0.0.1:${RUST_PORT}/mcp"
    exit 1
fi

echo "Rust baseline server ready."
echo ""

echo "=== Environment Snapshot ==="
uname -a
runcargo_version() { "$RUSTC_BIN" --version && "$CARGO_BIN" --version; }
runcargo_version
echo ""

echo "=== Running B2-http_performance against Rust baseline ==="
B2_BIN="$BUILD_DIR/bin/B2-http_performance"
if [ -x "$B2_BIN" ]; then
    "$B2_BIN" \
        --url "http://127.0.0.1:${RUST_PORT}/mcp" \
        --connections 8 \
        --requests 2000 \
        --io 2 \
        --compute 0
else
    echo "✗ $B2_BIN missing; skip Rust compare for B2."
fi
echo ""

echo "=== Running B3-concurrent_requests against Rust baseline ==="
B3_BIN="$BUILD_DIR/bin/B3-concurrent_requests"
if [ -x "$B3_BIN" ]; then
    "$B3_BIN" \
        --url "http://127.0.0.1:${RUST_PORT}/mcp" \
        --workers 10 \
        --requests 100
else
    echo "✗ $B3_BIN missing; skip Rust compare for B3."
fi
echo ""

echo "=== Rust baseline compare complete ==="
echo "Logs: $SERVER_LOG"
