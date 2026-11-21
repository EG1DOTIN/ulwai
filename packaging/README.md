# ULWAI AI Agent — Multi-Platform Portable Packaging Hub

This directory contains the complete source code, toolchains, build scripts, and documentation for creating **100% portable, self-contained setups** for **Linux** and **Windows** that can run from anywhere without complex installations or dependencies.

---

## 📁 Directory Architecture

```text
packaging/
├── README.md                      # Master packaging guide (this file)
│
├── dist/                          # Output directory for all compiled portable packages
│   ├── ULWAI-v2.0.0-x86_64.AppImage
│   ├── ulwai-v2.0.0-linux-x86_64-portable.tar.gz
│   ├── ulwai-v2.0.0-windows-x64-portable.zip
│   └── SHA256SUMS.txt
│
├── linux/                         # Linux Portable Setup (AppImage & Tarball)
│   ├── build_appimage.sh          # One-click automated AppImage generator
│   ├── ulwai.desktop              # XDG desktop integration entry
│   ├── AppRun                     # Portable dynamic library loader
│   └── README.md                  # Comprehensive Linux packaging documentation
│
└── windows/                       # Windows Portable Setup (MinGW-w64 & NSIS)
    ├── build_windows_mingw.bat    # Native Windows MinGW build & windeployqt script
    ├── build_windows_cross_linux.sh # Linux cross-compilation script via MinGW
    ├── mingw-w64-toolchain.cmake  # CMake cross-compiler toolchain file
    ├── ulwai_installer.nsi        # NSIS one-click setup script (ULWAI-setup.exe)
    └── README.md                  # Comprehensive Windows packaging documentation
```

---

## 🎯 Quick-Start Summary

### 🐧 To Build the Linux Portable AppImage:
```bash
./packaging/linux/build_appimage.sh
```
*Output: `packaging/dist/ULWAI-v2.0.0-x86_64.AppImage` (runs on Ubuntu, Debian, Fedora, Arch, etc.)*

### 🪟 To Build the Windows Portable ZIP & Installer:
- **On Windows (with MinGW + Qt):** Double-click or run `packaging\windows\build_windows_mingw.bat`
- **On Linux (cross-compile CLI):** Run `./packaging/windows/build_windows_cross_linux.sh`
- **Via GitHub Actions:** Push to `main` to trigger `.github/workflows/portable-builds.yml`

---

## 🔒 Portability & Zero-Dependency Guarantees
- **Linux AppImage:** Bundles its own Qt6 runtime, font fallback engines, platform plugins (`platforms/libqxcb.so`), image formats, and network TLS handlers.
- **Windows MinGW-w64:** 100% free and open-source GCC toolchain; all dependent Qt and compiler DLLs are packaged into the application root directory alongside the executables.
