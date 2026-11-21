# ULWAI AI Agent — macOS Packaging Guide (.app & .dmg)

This directory contains the automation, scripts, and documentation for building native **macOS Application Bundles (`.app`)** and **drag-and-drop Disk Images (`.dmg`)**.

---

## 📦 What is Created

Running the packaging tool produces two distributions in `packaging/dist/`:

1. **`ULWAI-v2.0.0-macos.dmg` (Disk Image Installer)**
   - The standard Apple disk image.
   - Users double-click the `.dmg` and drag `ULWAI.app` into their `/Applications` folder, or run it directly.
2. **`ulwai-v2.0.0-macos-portable.tar.gz` (Portable Bundle with CLI)**
   - Contains both the `ULWAI.app` Desktop GUI bundle and the standalone `ulwai-cli` Mach-O binary for pure terminal usage.

---

## 🏗️ macOS Bundle Architecture

Unlike Linux or Windows flat directories, macOS desktop applications are structured as self-contained directory trees called **Application Bundles**:

```text
ULWAI.app/
└── Contents/
    ├── Info.plist               # App metadata, bundle ID, retina display flags
    ├── MacOS/
    │   └── ulw-ai-agent         # Native compiled Mach-O binary
    ├── Resources/
    │   ├── ulwai.icns           # High-resolution Apple ICNS icon
    │   └── resources/           # App brand logos and graphics
    ├── Frameworks/              # Bundled Qt6 frameworks (via macdeployqt)
    │   ├── QtCore.framework
    │   ├── QtGui.framework
    │   └── QtWidgets.framework
    └── PlugIns/
        └── platforms/
            └── libqcocoa.dylib  # Native macOS Cocoa platform plugin
```

All dynamic library paths are automatically patched using Apple's `install_name_tool` so the app is 100% portable and does not depend on system-installed Qt.

---

## 🚀 How to Build on macOS (One Command)

From the project root on any Mac (Apple Silicon M1/M2/M3 or Intel):

1. Ensure **Homebrew** and **Qt 6** are installed:
   ```bash
   brew install qt@6 cmake
   ```
2. Run the build script:
   ```bash
   ./packaging/macos/build_macos.sh
   ```
3. The script will:
   - Compile `ulw-ai-agent` and `ulwai-cli` with Apple Clang (`-O3`).
   - Generate `ulwai.icns` using Apple's native `iconutil`.
   - Run Qt's `macdeployqt` to bundle all Cocoa plugins and frameworks.
   - Package `packaging/dist/ULWAI-v2.0.0-macos.dmg`.

---

## 🤖 Automated Cloud Builds (GitHub Actions)

You don't need to own a Mac to generate macOS builds! GitHub Actions provides free `macos-latest` (Apple Silicon M1/arm64) runners. 

Our workflow (`.github/workflows/ci.yml`) runs the macOS job automatically in the cloud on every push or tag release, uploading the `.dmg` to GitHub Releases.

---

## 🔒 Apple Gatekeeper / Quarantine Note

Because open-source binaries downloaded from web browsers are not signed with a paid Apple Developer Certificate (\$99/year), macOS Gatekeeper may display a prompt saying:
> *"ULWAI.app can't be opened because Apple cannot check it for malicious software."*

### How to Open:
1. **Option A (GUI):** Right-click (or Control-click) `ULWAI.app` $\rightarrow$ Click **Open** $\rightarrow$ Click **Open** again in the confirmation prompt.
2. **Option B (Terminal):** Clear the quarantine attribute:
   ```bash
   xattr -cr /Applications/ULWAI.app
   ```
