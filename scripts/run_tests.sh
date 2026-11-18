#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running build.sh first..."
    "${SCRIPT_DIR}/build.sh"
fi

VERSION=$(tr -d '\r\n' < "${ROOT_DIR}/VERSION" 2>/dev/null || echo "2.0.0")

echo "=================================================="
echo " Running ULWAI AI Agent v${VERSION} Test Suite"
echo "=================================================="

ctest --test-dir "${BUILD_DIR}" --output-on-failure
"${BUILD_DIR}/tests/ulwai_tests"
