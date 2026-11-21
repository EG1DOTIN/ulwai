#!/usr/bin/env bash
# ==============================================================================
# ULWAI AI Agent - macOS Portable Packaging Script (.app & .dmg)
# Builds native macOS application bundle and standalone disk image (.dmg).
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DIST_DIR="${ROOT_DIR}/packaging/dist"
BUILD_DIR="${ROOT_DIR}/build-macos"

if [ -f "${ROOT_DIR}/VERSION" ]; then
    VERSION=$(tr -d '[:space:]' < "${ROOT_DIR}/VERSION")
else
    VERSION="2.0.0"
fi

echo "================================================================="
echo " Packaging ULWAI AI Agent v${VERSION} for macOS (.app & .dmg)"
echo "================================================================="

mkdir -p "${DIST_DIR}"
mkdir -p "${BUILD_DIR}"

# 1. Locate Qt6 on macOS via Homebrew
QT_PREFIX=""
if command -v brew &>/dev/null; then
    QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null || true)"
fi

if [ -z "${QT_PREFIX}" ] || [ ! -d "${QT_PREFIX}" ]; then
    echo "[!] Qt6 not found via Homebrew. Searching common macOS paths..."
    for candidate in /opt/homebrew/opt/qt@6 /opt/homebrew/opt/qt /usr/local/opt/qt@6 /usr/local/opt/qt; do
        if [ -d "$candidate" ]; then
            QT_PREFIX="$candidate"
            break
        fi
    done
fi

if [ -n "${QT_PREFIX}" ]; then
    echo "  -> Found Qt6 at: ${QT_PREFIX}"
    export PATH="${QT_PREFIX}/bin:${PATH}"
    export CMAKE_PREFIX_PATH="${QT_PREFIX}:${CMAKE_PREFIX_PATH:-}"
else
    echo "[WARNING] Qt6 prefix could not be automatically resolved. Ensure Qt6 is in PATH."
fi

# 2. Generate macOS Application Icon (.icns)
echo "[1/4] Generating macOS application icon (.icns)..."
ICONSET_DIR="${BUILD_DIR}/ulwai.iconset"
mkdir -p "${ICONSET_DIR}"
RESOURCES_DIR="${ROOT_DIR}/resources/icons"

if [ -f "${RESOURCES_DIR}/ulwai-icon-512.png" ] && command -v iconutil &>/dev/null; then
    cp "${RESOURCES_DIR}/ulwai-icon-32.png"  "${ICONSET_DIR}/icon_16x16@2x.png"  2>/dev/null || true
    cp "${RESOURCES_DIR}/ulwai-icon-32.png"  "${ICONSET_DIR}/icon_32x32.png"     2>/dev/null || true
    cp "${RESOURCES_DIR}/ulwai-icon-64.png"  "${ICONSET_DIR}/icon_32x32@2x.png"  2>/dev/null || true
    cp "${RESOURCES_DIR}/ulwai-icon-128.png" "${ICONSET_DIR}/icon_128x128.png"   2>/dev/null || true
    cp "${RESOURCES_DIR}/ulwai-icon-512.png" "${ICONSET_DIR}/icon_256x256@2x.png" 2>/dev/null || true
    cp "${RESOURCES_DIR}/ulwai-icon-512.png" "${ICONSET_DIR}/icon_512x512.png"   2>/dev/null || true
    iconutil -c icns "${ICONSET_DIR}" -o "${RESOURCES_DIR}/ulwai.icns" 2>/dev/null || true
fi

# 3. Configure and Compile Release Targets
echo "[2/4] Configuring and Compiling with Apple Clang..."
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GUI=ON \
    -DBUILD_TESTS=OFF \
    ${QT_PREFIX:+-DCMAKE_PREFIX_PATH="${QT_PREFIX}"}

cmake --build "${BUILD_DIR}" --config Release -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# 4. Deploy Qt Frameworks via macdeployqt & Generate .dmg
echo "[3/4] Deploying Qt frameworks and creating DMG..."
APP_BUNDLE="${BUILD_DIR}/ulw-ai-agent.app"

# Copy icon and resources into bundle if not placed by CMake
if [ -d "${APP_BUNDLE}" ]; then
    mkdir -p "${APP_BUNDLE}/Contents/Resources"
    if [ -f "${RESOURCES_DIR}/ulwai.icns" ]; then
        cp "${RESOURCES_DIR}/ulwai.icns" "${APP_BUNDLE}/Contents/Resources/"
    fi
    if [ -d "${ROOT_DIR}/resources" ]; then
        cp -r "${ROOT_DIR}/resources" "${APP_BUNDLE}/Contents/Resources/"
    fi

    # Run official macdeployqt tool to bundle Qt frameworks and create DMG
    MACDEPLOYQT="${QT_PREFIX}/bin/macdeployqt"
    if [ ! -x "${MACDEPLOYQT}" ]; then
        MACDEPLOYQT="$(command -v macdeployqt || true)"
    fi

    if [ -n "${MACDEPLOYQT}" ] && [ -x "${MACDEPLOYQT}" ]; then
        echo "  -> Running ${MACDEPLOYQT} -dmg ..."
        "${MACDEPLOYQT}" "${APP_BUNDLE}" -always-overwrite -dmg || true
    fi

    # Locate generated DMG dynamically
    DMG_GENERATED=$(find "${BUILD_DIR}" -maxdepth 2 -name "*.dmg" -print -quit 2>/dev/null || true)
    FINAL_DMG="${DIST_DIR}/ULWAI-v${VERSION}-macos.dmg"
    if [ -n "${DMG_GENERATED}" ] && [ -f "${DMG_GENERATED}" ]; then
        mv "${DMG_GENERATED}" "${FINAL_DMG}"
        echo "  -> Created DMG: ${FINAL_DMG}"
    fi
fi

# 5. Create Standalone Portable macOS Tarball (.tar.gz)
echo "[4/4] Creating portable macOS tarball with CLI..."
PORTABLE_STAGE="${BUILD_DIR}/stage/ulwai-v${VERSION}-macos-portable"
mkdir -p "${PORTABLE_STAGE}"

if [ -d "${APP_BUNDLE}" ]; then
    cp -R "${APP_BUNDLE}" "${PORTABLE_STAGE}/"
fi
if [ -f "${BUILD_DIR}/ulwai-cli" ]; then
    cp "${BUILD_DIR}/ulwai-cli" "${PORTABLE_STAGE}/"
fi
cp "${ROOT_DIR}/README.md" "${ROOT_DIR}/LICENSE" "${PORTABLE_STAGE}/"

TAR_OUTPUT="${DIST_DIR}/ulwai-v${VERSION}-macos-portable.tar.gz"
tar -czf "${TAR_OUTPUT}" -C "${BUILD_DIR}/stage" "ulwai-v${VERSION}-macos-portable"

# Generate Checksums
cd "${DIST_DIR}"
if command -v sha256sum &>/dev/null; then
    sha256sum ULWAI-v* ulwai-v* > SHA256SUMS.txt 2>/dev/null || true
elif command -v shasum &>/dev/null; then
    shasum -a 256 ULWAI-v* ulwai-v* > SHA256SUMS.txt 2>/dev/null || true
fi

echo "================================================================="
echo " macOS Portable Packaging Complete!"
echo " Output files in packaging/dist/:"
ls -lh "${DIST_DIR}"
echo "================================================================="
