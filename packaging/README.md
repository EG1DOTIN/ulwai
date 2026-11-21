# ULWAI AI Agent — Multi-Platform Portable Packaging Hub

This directory contains the complete source code, toolchains, build scripts, and documentation for creating **100% portable, self-contained setups** for **Linux**, **Windows**, and **macOS** that can run from anywhere without complex installations or missing dependencies.

---

## 📁 Directory Architecture

```text
packaging/
├── README.md                          # Master packaging guide (this file)
│
├── dist/                              # Output directory for all compiled portable packages
│   ├── ULWAI-v2.0.0-x86_64.AppImage   # Linux AppImage (runs on all Linux distros)
│   ├── ulwai-v2.0.0-linux-x86_64-...  # Linux Portable Tarball
│   ├── ulwai-v2.0.0-windows-x64-...   # Windows Portable ZIP (MinGW-w64)
│   ├── ULWAI-v2.0.0-macos.dmg         # macOS Drag-and-Drop Disk Image
│   ├── ulwai-v2.0.0-macos-portable... # macOS Portable Tarball (App + CLI)
│   └── SHA256SUMS.txt                 # Cryptographic verification hashes
│
├── linux/                             # Linux Portable Setup (AppImage & Tarball)
│   ├── build_appimage.sh              # One-click automated AppImage generator
│   ├── ulwai.desktop                  # XDG desktop integration entry
│   ├── AppRun                         # Portable dynamic library loader
│   └── README.md                      # Comprehensive Linux packaging documentation
│
├── windows/                           # Windows Portable Setup (MinGW-w64 & NSIS)
│   ├── build_windows_mingw.bat        # Native Windows MinGW build & windeployqt script
│   ├── build_windows_cross_linux.sh   # Linux cross-compilation script via MinGW
│   ├── mingw-w64-toolchain.cmake      # CMake cross-compiler toolchain file
│   ├── ulwai_installer.nsi            # NSIS one-click setup script (ULWAI-setup.exe)
│   └── README.md                      # Comprehensive Windows packaging documentation
│
└── macos/                             # macOS Portable Setup (.app & .dmg)
    ├── build_macos.sh                 # One-click automated macOS App & DMG builder
    └── README.md                      # Comprehensive macOS packaging documentation
```

---

## 🎯 Quick-Start Summary

### 🐧 Linux Portable (AppImage & Tarball):
```bash
./packaging/linux/build_appimage.sh
```
*Output: `packaging/dist/ULWAI-v2.0.0-x86_64.AppImage` (runs on Ubuntu, Debian, Fedora, Arch, Mint, etc.)*

### 🪟 Windows Portable (MinGW-w64 ZIP & Setup):
- **On Windows (with MinGW + Qt):** Run `packaging\windows\build_windows_mingw.bat`
- **On Linux (cross-compile CLI):** Run `./packaging/windows/build_windows_cross_linux.sh`
- **Via GitHub Actions:** Builds automatically on `windows-latest` using MSYS2 UCRT64.

### 🍎 macOS Portable (.dmg & .app):
- **On macOS (with Homebrew + Qt):** Run `./packaging/macos/build_macos.sh`
- **Via GitHub Actions:** Builds automatically on `macos-latest` (Apple Silicon M1/arm64) using `macdeployqt`.

---

## 🔒 Portability & Zero-Dependency Guarantees
- **Linux AppImage:** Bundles its own Qt6 runtime, font fallback engines, platform plugins (`platforms/libqxcb.so`, `wayland`), image formats, and network TLS handlers.
- **Windows MinGW-w64:** 100% free and open-source GCC toolchain; all dependent Qt and compiler DLLs are packaged into the application root directory alongside the executables.
- **macOS DMG:** Self-contained application bundle (`ULWAI.app`) with all Cocoa plugins and Qt frameworks bundled in `Contents/Frameworks/` via `macdeployqt`.
