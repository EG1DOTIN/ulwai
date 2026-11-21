# ULWAI AI Agent — Linux Portable Packaging Guide

This directory contains the complete tooling and automation for building **100% self-contained portable AppImages** and **standalone tarballs** for Linux.

---

## 📦 What is Created

When you run `build_appimage.sh`, two portable distributions are generated inside `packaging/dist/`:

1. **`ULWAI-v2.0.0-x86_64.AppImage` (~40MB)**
   - Single executable binary containing the application, Qt6 GUI libraries, network backends, and platform plugins.
   - Runs directly on **any 64-bit Linux distribution** (Ubuntu, Debian, Fedora, Arch Linux, Linux Mint, openSUSE, RHEL) with zero installation and zero root/sudo privileges required.
2. **`ulwai-v2.0.0-linux-x86_64-portable.tar.gz` (~43MB)**
   - Portable folder archive with a standalone `run.sh` launcher script that sets isolated library paths.

---

## 🚀 How to Build the AppImage (One Command)

From the project root directory, simply run:

```bash
./packaging/linux/build_appimage.sh
```

### Build Steps Performed Automatically:
1. **Compilation:** Compiles all Release binaries (`ulw-ai-agent`, `ulwai`, `ulwai-cli`) using CMake with `-O3` optimization.
2. **AppDir Staging:** Assembles the standard XDG AppDir layout (`usr/bin`, `usr/lib`, `usr/plugins`, desktop entry, and 512px icon).
3. **Library Resolution:** Inspects dynamic dependencies via `ldd` and copies required Qt6 (`libQt6Widgets`, `libQt6Gui`, `libQt6Core`), `libcurl`, `libssl`, and `libcrypto` libraries into the bundle, while excluding host glibc to prevent version collisions.
4. **Qt Plugin Deployment:** Bundles `platforms/libqxcb.so`, `wayland` integration, TLS network backends, and image format handlers.
5. **Binary Stripping:** Strips unneeded debug symbols to maintain an ultra-lightweight footprint.
6. **AppImage Assembly:** Uses the standalone `appimagetool` to compress everything into a signed SquashFS image.
7. **Checksum Generation:** Produces SHA-256 hashes in `packaging/dist/SHA256SUMS.txt`.

---

## 🖥️ How to Run the AppImage

### Graphical Desktop Mode:
```bash
chmod +x packaging/dist/ULWAI-v2.0.0-x86_64.AppImage
./packaging/dist/ULWAI-v2.0.0-x86_64.AppImage
```

### Pure Terminal CLI Mode:
```bash
./packaging/dist/ULWAI-v2.0.0-x86_64.AppImage --cli
```

### Inspecting Contents (Extract):
```bash
./packaging/dist/ULWAI-v2.0.0-x86_64.AppImage --appimage-extract
ls -la squashfs-root/
```

---

## 🛠️ File Structure

| File | Purpose |
| :--- | :--- |
| `build_appimage.sh` | Main automated build script. |
| `ulwai.desktop` | Standard XDG Desktop entry specifying application name, categories, and icons. |
| `AppRun` | Entry point script executed by the AppImage runtime to configure library paths. |
| `README.md` | This technical guide. |
