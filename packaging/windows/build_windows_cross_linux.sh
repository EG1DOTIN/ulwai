#!/usr/bin/env bash
# ==============================================================================
# ULWAI AI Agent - Windows Cross-Compilation Script (Run from Linux)
# Uses 100% free & open-source MinGW-w64 (x86_64-w64-mingw32-g++).
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DIST_DIR="${ROOT_DIR}/packaging/dist"
STAGE_DIR="${ROOT_DIR}/build/windows-stage"

echo "================================================================="
echo " Building ULWAI for Windows via MinGW-w64 (Cross-Compile)"
echo "================================================================="

# Check if MinGW-w64 cross-compiler is installed
if ! command -v x86_64-w64-mingw32-g++ &>/dev/null; then
    echo "[!] MinGW-w64 cross compiler was not found on your Linux system."
    echo "    To install it on Debian/Ubuntu, run:"
    echo "      sudo apt update && sudo apt install -y mingw-w64 g++-mingw-w64-x86-64"
    echo ""
    echo "[*] For full automated builds (Desktop GUI + CLI + Installer), use the"
    echo "    included GitHub Actions workflow (.github/workflows/portable-builds.yml)"
    echo "    or run 'packaging/windows/build_windows_mingw.bat' on a Windows machine."
    exit 1
fi

mkdir -p "${STAGE_DIR}"
mkdir -p "${DIST_DIR}"

BUILD_DIR="${ROOT_DIR}/build-win64"
mkdir -p "${BUILD_DIR}"

echo "[1/3] Configuring CMake with MinGW-w64 toolchain..."
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/mingw-w64-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
    -DBUILD_GUI=OFF

echo "[2/3] Compiling standalone Windows CLI executable..."
cmake --build "${BUILD_DIR}" --target ulwai-cli -j"$(nproc)"

echo "[3/3] Assembling portable Windows package..."
cp "${BUILD_DIR}/ulwai-cli.exe" "${STAGE_DIR}/"
cp "${ROOT_DIR}/README.md" "${ROOT_DIR}/LICENSE" "${STAGE_DIR}/"

if [ -f "${ROOT_DIR}/VERSION" ]; then
    VERSION=$(tr -d '[:space:]' < "${ROOT_DIR}/VERSION")
else
    VERSION="2.0.0"
fi

ZIP_FILE="${DIST_DIR}/ulwai-v${VERSION}-windows-cli-portable.zip"
cd "${STAGE_DIR}"
zip -9 -r "${ZIP_FILE}" ./*

echo "================================================================="
echo " Windows CLI Portable Package Created: ${ZIP_FILE}"
echo "================================================================="
