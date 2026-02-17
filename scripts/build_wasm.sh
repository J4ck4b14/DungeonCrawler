#!/usr/bin/env bash
# Build script for Emscripten WebAssembly target
# Requires emsdk activated (emcmake / emmake available in PATH)
set -e
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-wasm"
WEB_DIR="$ROOT_DIR/web/game"

echo "Configuring Emscripten build in: $BUILD_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

emcmake cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja
emmake cmake --build "$BUILD_DIR" --config Release

echo "Build finished. Web files should be in: $WEB_DIR"

echo "If you want to copy the web files into another website, run:"
echo "  cp -r $WEB_DIR /path/to/your/site/game"
