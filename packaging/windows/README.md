# ULWAI AI Agent — Windows MinGW-w64 Packaging Guide

This directory contains the complete toolset to build and package **portable Windows distributions** and **one-click installers** using **MinGW-w64** (Minimalist GNU for Windows 64-bit).

---

## 🌟 Why MinGW-w64? (100% Free & Open Source)

Unlike Microsoft Visual Studio (MSVC), which is proprietary, requires multi-gigabyte installations, and imposes restrictive licensing:
- **100% Free & Open Source:** MinGW-w64 is powered by standard GNU GCC (`gcc`, `g++`).
- **No Visual Studio Required:** Compiles native 64-bit PE/COFF Windows executables (`.exe`) directly.
- **Portable & Lightweight:** Uses static C/C++ runtimes (`-static-libgcc -static-libstdc++`) or standard Windows `msvcrt.dll`.
- **Cross-Compilable:** Can be run directly on Windows or cross-compiled from Linux!

---

## 📦 What is Created

Running the packaging tools produces:

1. **`ulwai-v2.0.0-windows-x64-portable.zip`**
   - Self-contained directory containing `ulw-ai-agent.exe`, `ulwai-cli.exe`, and all runtime dependencies (`Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `platforms/qwindows.dll`, `libcurl.dll`).
   - The user extracts the `.zip` anywhere (Desktop, USB drive, Program Files) and double-clicks `ulw-ai-agent.exe` to run.
2. **`ULWAI-v2.0.0-windows-setup.exe` (NSIS Installer)**
   - Professional one-click installer.
   - Installs to user directory (`%LOCALAPPDATA%\Programs\ULWAI`) without requiring administrative permissions.
   - Creates Start Menu shortcuts, Desktop icon, and a clean uninstaller in Windows "Installed Apps".

---

## 🚀 Option 1: Native Build on Windows (Recommended for GUI)

If you are on a Windows machine with MinGW-w64 and Qt installed:

1. Open a Command Prompt or PowerShell in this directory.
2. Run:
   ```cmd
   build_windows_mingw.bat
   ```
3. The script will:
   - Configure CMake with `-G "MinGW Makefiles"`
   - Compile `ulw-ai-agent.exe` and `ulwai-cli.exe`
   - Run `windeployqt` to automatically collect all Qt DLLs and plugins
   - Create `packaging/dist/ulwai-v2.0.0-windows-x64-portable.zip`
   - (If NSIS is installed) Compile `packaging/dist/ULWAI-v2.0.0-windows-setup.exe`

---

## 🐧 Option 2: Cross-Compiling from Linux

You can cross-compile the standalone Windows CLI directly from Ubuntu/Debian:

1. Install the MinGW-w64 cross-compiler:
   ```bash
   sudo apt update
   sudo apt install -y mingw-w64 g++-mingw-w64-x86-64 zip
   ```
2. Run the cross-compilation script:
   ```bash
   ./packaging/windows/build_windows_cross_linux.sh
   ```
3. This creates `packaging/dist/ulwai-v2.0.0-windows-cli-portable.zip` with the native `ulwai-cli.exe` executable.

---

## 🤖 Option 3: Automated Continuous Integration (GitHub Actions)

We provide an automated GitHub Actions workflow (`.github/workflows/portable-builds.yml`) that runs on `windows-latest` with MinGW-w64 to build the portable ZIP and installer automatically in the cloud on every release.

---

## 🛠️ File Structure

| File | Purpose |
| :--- | :--- |
| `build_windows_mingw.bat` | Windows batch script to build, deploy Qt DLLs, and package the ZIP. |
| `build_windows_cross_linux.sh` | Linux bash script for cross-compiling via MinGW. |
| `mingw-w64-toolchain.cmake` | CMake toolchain file defining `x86_64-w64-mingw32` compiler targets. |
| `ulwai_installer.nsi` | NSIS script to generate the one-click `ULWAI-windows-setup.exe` installer. |
| `README.md` | This technical guide. |
