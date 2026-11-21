#!/usr/bin/env bash
# ==============================================================================
# ULWAI AI Agent - Linux Portable AppImage & Tarball Build Script
# Creates a 100% self-contained portable AppImage that runs on any Linux distro.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DIST_DIR="${ROOT_DIR}/packaging/dist"
APPDIR="${ROOT_DIR}/build/AppDir"

# 1. Determine Project Version
if [ -f "${ROOT_DIR}/VERSION" ]; then
    VERSION=$(tr -d '[:space:]' < "${ROOT_DIR}/VERSION")
else
    VERSION="2.0.0"
fi

echo "================================================================="
echo " Packaging ULWAI AI Agent v${VERSION} (Linux Portable AppImage)"
echo "================================================================="

# 2. Ensure CMake Project is Compiled in Release Mode
echo "[1/6] Building ULWAI Release binaries..."
cmake -B "${ROOT_DIR}/build" -S "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GUI=ON \
    -DBUILD_TESTS=OFF

cmake --build "${ROOT_DIR}/build" --config Release -j"$(nproc)"

# 3. Create Fresh AppDir Directory Hierarchy
echo "[2/6] Assembling AppDir structure..."
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib"
mkdir -p "${APPDIR}/usr/plugins/platforms"
mkdir -p "${APPDIR}/usr/plugins/imageformats"
mkdir -p "${APPDIR}/usr/plugins/tls"
mkdir -p "${APPDIR}/usr/plugins/xcbglintegrations"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/512x512/apps"
mkdir -p "${APPDIR}/resources/icons"
mkdir -p "${DIST_DIR}"

# 4. Copy Executables & Resources
echo "[3/6] Copying binaries and brand assets..."
cp "${ROOT_DIR}/build/ulw-ai-agent" "${APPDIR}/usr/bin/"
cp "${ROOT_DIR}/build/ulwai" "${APPDIR}/usr/bin/"
cp "${ROOT_DIR}/build/ulwai-cli" "${APPDIR}/usr/bin/"
chmod +x "${APPDIR}/usr/bin/"*

# App resources
if [ -d "${ROOT_DIR}/resources" ]; then
    cp -r "${ROOT_DIR}/resources/"* "${APPDIR}/resources/"
fi

# Desktop file and icons
cp "${SCRIPT_DIR}/ulwai.desktop" "${APPDIR}/ulwai.desktop"
cp "${SCRIPT_DIR}/ulwai.desktop" "${APPDIR}/usr/share/applications/"
cp "${ROOT_DIR}/resources/icons/ulwai-icon-512.png" "${APPDIR}/ulwai.png"
cp "${ROOT_DIR}/resources/icons/ulwai-icon-512.png" "${APPDIR}/.DirIcon"
cp "${ROOT_DIR}/resources/icons/ulwai-icon-512.png" "${APPDIR}/usr/share/icons/hicolor/512x512/apps/ulwai.png"

# AppRun script
cp "${SCRIPT_DIR}/AppRun" "${APPDIR}/AppRun"
chmod +x "${APPDIR}/AppRun"

# 5. Collect Dynamic Shared Libraries & Qt Plugins
echo "[4/6] Collecting Qt6 plugins and dependent shared libraries..."

QT_PLUGIN_BASE=""
for p in /usr/lib/x86_64-linux-gnu/qt6/plugins /usr/lib64/qt6/plugins /usr/lib/qt6/plugins; do
    if [ -d "$p" ]; then
        QT_PLUGIN_BASE="$p"
        break
    fi
done

if [ -n "${QT_PLUGIN_BASE}" ]; then
    echo "  -> Found Qt6 plugin directory: ${QT_PLUGIN_BASE}"
    # Platforms (xcb, wayland)
    if [ -d "${QT_PLUGIN_BASE}/platforms" ]; then
        cp -L "${QT_PLUGIN_BASE}/platforms/"*.so "${APPDIR}/usr/plugins/platforms/" 2>/dev/null || true
    fi
    # Image formats
    if [ -d "${QT_PLUGIN_BASE}/imageformats" ]; then
        cp -L "${QT_PLUGIN_BASE}/imageformats/"*.so "${APPDIR}/usr/plugins/imageformats/" 2>/dev/null || true
    fi
    # TLS backends (OpenSSL, certonly)
    if [ -d "${QT_PLUGIN_BASE}/tls" ]; then
        cp -L "${QT_PLUGIN_BASE}/tls/"*.so "${APPDIR}/usr/plugins/tls/" 2>/dev/null || true
    fi
    # XCB GL integrations
    if [ -d "${QT_PLUGIN_BASE}/xcbglintegrations" ]; then
        cp -L "${QT_PLUGIN_BASE}/xcbglintegrations/"*.so "${APPDIR}/usr/plugins/xcbglintegrations/" 2>/dev/null || true
    fi
fi

# Copy all dependent libraries (excluding basic glibc to prevent host glibc collision)
echo "  -> Resolving dynamic linker dependencies..."
TARGET_BINARIES=("${APPDIR}/usr/bin/ulw-ai-agent" "${APPDIR}/usr/plugins/platforms/"*.so)
COLLECTED_LIBS=()

for bin in "${TARGET_BINARIES[@]}"; do
    if [ -f "$bin" ]; then
        while read -r line; do
            libpath=$(echo "$line" | grep -o '/[^ ]*\.so[^ ]*' || true)
            if [ -n "$libpath" ] && [ -f "$libpath" ]; then
                libname=$(basename "$libpath")
                # Skip core glibc and proprietary display drivers that must match host
                case "$libname" in
                    libc.so*|libm.so*|libdl.so*|librt.so*|libpthread.so*|libresolv.so*|ld-linux*|libGLX*|libGL.so*|libdrm*|libGLdispatch*)
                        continue
                        ;;
                    *)
                        if [ ! -f "${APPDIR}/usr/lib/${libname}" ]; then
                            cp -L "$libpath" "${APPDIR}/usr/lib/${libname}"
                        fi
                        ;;
                esac
            fi
        done < <(ldd "$bin" 2>/dev/null || true)
    fi
done

# Strip debugging symbols from bundled binaries to maintain lightweight footprint
strip --strip-unneeded "${APPDIR}/usr/bin/"* 2>/dev/null || true
strip --strip-unneeded "${APPDIR}/usr/lib/"*.so* 2>/dev/null || true

# 6. Build AppImage Using appimagetool
echo "[5/6] Packaging AppImage..."
TOOLS_DIR="${ROOT_DIR}/build/tools"
mkdir -p "${TOOLS_DIR}"

APPIMAGETOOL="${TOOLS_DIR}/appimagetool"
if [ ! -f "${APPIMAGETOOL}" ]; then
    echo "  -> Downloading standalone appimagetool..."
    curl -sL "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" -o "${APPIMAGETOOL}"
    chmod +x "${APPIMAGETOOL}"
fi

APPIMAGE_OUTPUT="${DIST_DIR}/ULWAI-v${VERSION}-x86_64.AppImage"
rm -f "${APPIMAGE_OUTPUT}"

export ARCH="x86_64"
export APPIMAGE_EXTRACT_AND_RUN=1

# Run appimagetool with NO_STRIP to avoid altering pre-stripped libraries
ARCH=x86_64 "${APPIMAGETOOL}" --appimage-extract-and-run "${APPDIR}" "${APPIMAGE_OUTPUT}"

chmod +x "${APPIMAGE_OUTPUT}"

# 7. Also Create Portable Self-Contained Tarball (.tar.gz)
echo "[6/6] Creating standalone portable tarball..."
PORTABLE_DIR_NAME="ulwai-v${VERSION}-linux-x86_64-portable"
PORTABLE_STAGE="${ROOT_DIR}/build/${PORTABLE_DIR_NAME}"
rm -rf "${PORTABLE_STAGE}"
mkdir -p "${PORTABLE_STAGE}"

cp -r "${APPDIR}/usr/bin" "${PORTABLE_STAGE}/"
cp -r "${APPDIR}/usr/lib" "${PORTABLE_STAGE}/"
cp -r "${APPDIR}/usr/plugins" "${PORTABLE_STAGE}/"
cp -r "${APPDIR}/resources" "${PORTABLE_STAGE}/"
cp "${ROOT_DIR}/README.md" "${ROOT_DIR}/LICENSE" "${PORTABLE_STAGE}/"

# Write standalone shell launcher
cat << 'EOF' > "${PORTABLE_STAGE}/run.sh"
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${SCRIPT_DIR}/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${SCRIPT_DIR}/plugins/platforms"

if [ "$1" = "--cli" ]; then
    exec "${SCRIPT_DIR}/bin/ulwai-cli" "$@"
else
    exec "${SCRIPT_DIR}/bin/ulw-ai-agent" "$@"
fi
EOF
chmod +x "${PORTABLE_STAGE}/run.sh"

TAR_OUTPUT="${DIST_DIR}/${PORTABLE_DIR_NAME}.tar.gz"
tar -czf "${TAR_OUTPUT}" -C "${ROOT_DIR}/build" "${PORTABLE_DIR_NAME}"

# Generate Checksums
cd "${DIST_DIR}"
sha256sum "$(basename "${APPIMAGE_OUTPUT}")" "$(basename "${TAR_OUTPUT}")" > SHA256SUMS.txt

echo "================================================================="
echo " Portable Linux Packages Successfully Created in packaging/dist:"
echo "   1. AppImage: $(basename "${APPIMAGE_OUTPUT}") ($(du -h "${APPIMAGE_OUTPUT}" | cut -f1))"
echo "   2. Tarball:  $(basename "${TAR_OUTPUT}") ($(du -h "${TAR_OUTPUT}" | cut -f1))"
echo "   3. Hashes:   SHA256SUMS.txt"
echo "================================================================="
