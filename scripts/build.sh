#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

BUILD_TYPE="${1:-Release}"
VERSION=$(tr -d '\r\n' < "${ROOT_DIR}/VERSION" 2>/dev/null || echo "2.0.0")

echo "=================================================="
echo " Building ULWAI AI Agent v${VERSION} (${BUILD_TYPE})"
echo " Root: ${ROOT_DIR}"
echo "=================================================="

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_TESTS=ON \
    -DBUILD_GUI=ON

cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo ">>> Build completed successfully in ${BUILD_DIR} <<<"
