/**
 * @file Platform.cpp
 * @brief Cross-Platform Operating System Abstraction Layer for ULWAI.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Provides clean, isolated, cross-platform OS abstractions across Linux, macOS, and Windows.
 *   Ensures that business logic, UI code, and CLI runners never make direct platform-dependent
 *   system calls or assume hardcoded filesystem paths.
 *
 * RESPONSIBILITIES:
 *   1. Standard Directory Resolution:
 *      - Config directory: `~/.config/ulwai` (Linux/macOS respecting `$XDG_CONFIG_HOME`), or `%APPDATA%\ulwai` (Windows).
 *      - History directory: `<config_dir>/history/`.
 *      - User home directory: `$HOME` / `getpwuid` (Unix) or `%USERPROFILE%` / `%HOMEDRIVE%%HOMEPATH%` (Windows).
 *   2. Command Pipe Execution (`executeCommand`):
 *      - Spawns child subshells via POSIX `popen` or Win32 `_popen`.
 *      - Intercepts and merges `stdout` and `stderr` (`2>&1`) to capture all command output.
 *      - Retrieves accurate process exit codes via `pclose` and `WEXITSTATUS`.
 *   3. Clipboard Access (`copyToClipboard`):
 *      - Linux: Tries `wl-copy` (Wayland), falling back to `xclip` or `xsel` (X11).
 *      - macOS: Pipes directly into `pbcopy`.
 *      - Windows: Uses Win32 Clipboard APIs (`OpenClipboard`, `EmptyClipboard`, `SetClipboardData`).
 *   4. Default Text Editor Spawning (`openFileInDefaultEditor`):
 *      - Unix: Respects `$VISUAL` and `$EDITOR`, falling back to `xdg-open`, `nano`, or `vim`.
 *      - macOS: Launches via `open -t`.
 *      - Windows: Launches via `notepad.exe`.
 *   5. Console UTF-8 & ANSI Initialization (`initConsole`):
 *      - Configures Windows conhost for UTF-8 code page (65001) and enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING`
 *        for full ANSI 256-color support.
 */

#include "ulwai/Platform.hpp"
#include <cstdlib>
#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#endif

namespace ulwai::platform {

std::filesystem::path getHomeDirectory() {
#if defined(_WIN32)
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile != '\0') {
        return std::filesystem::path(userProfile);
    }
    const char* homeDrive = std::getenv("HOMEDRIVE");
    const char* homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath) {
        return std::filesystem::path(std::string(homeDrive) + homePath);
    }
    return std::filesystem::current_path();
#else
    const char* home = std::getenv("HOME");
    if (home && *home != '\0') {
        return std::filesystem::path(home);
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return std::filesystem::path(pw->pw_dir);
    }
    return std::filesystem::current_path();
#endif
}

std::filesystem::path getConfigDirectory() {
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData && *appData != '\0') {
        return std::filesystem::path(appData) / "ulwai";
    }
    return getHomeDirectory() / ".ulwai";
#else
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    std::filesystem::path base = (xdgConfig && *xdgConfig != '\0')
                                     ? std::filesystem::path(xdgConfig)
                                     : (getHomeDirectory() / ".config");
    return base / "ulwai";
#endif
}

std::filesystem::path getHistoryDirectory() {
    return getConfigDirectory() / "history";
}

std::filesystem::path getDefaultConfigFilePath() {
    return getConfigDirectory() / "config.json";
}

int executeCommand(const std::string& command, std::string& output) {
    output.clear();
    // Redirect stderr to stdout so we capture all command output and diagnostics
    std::string fullCmd = command + " 2>&1";
#if defined(_WIN32)
    FILE* pipe = _popen(fullCmd.c_str(), "r");
#else
    FILE* pipe = popen(fullCmd.c_str(), "r");
#endif

    if (!pipe) {
        output = "Failed to spawn command pipe";
        return -1;
    }

    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output.append(buffer.data());
    }

#if defined(_WIN32)
    int status = _pclose(pipe);
#else
    int status = pclose(pipe);
    if (WIFEXITED(status)) {
        status = WEXITSTATUS(status);
    }
#endif

    return status;
}

void copyToClipboard(const std::string& text) {
#if defined(_WIN32)
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlob) {
        void* pGlob = GlobalLock(hGlob);
        if (pGlob) {
            std::memcpy(pGlob, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
    }
    CloseClipboard();
#elif defined(__APPLE__)
    FILE* pipe = popen("pbcopy", "w");
    if (pipe) {
        std::fwrite(text.data(), 1, text.size(), pipe);
        pclose(pipe);
    }
#else
    // Linux: try wl-copy (Wayland), then xclip, then xsel
    FILE* pipe = popen("wl-copy 2>/dev/null || xclip -selection clipboard 2>/dev/null || xsel --clipboard --input 2>/dev/null", "w");
    if (pipe) {
        std::fwrite(text.data(), 1, text.size(), pipe);
        pclose(pipe);
    }
#endif
}

void openFileInDefaultEditor(const std::filesystem::path& path) {
    std::string p = path.string();
#if defined(_WIN32)
    std::string cmd = "notepad \"" + p + "\"";
#elif defined(__APPLE__)
    std::string cmd = "open -t \"" + p + "\"";
#else
    const char* visual = std::getenv("VISUAL");
    const char* editor = std::getenv("EDITOR");
    std::string ed = visual ? visual : (editor ? editor : "");
    std::string cmd;
    if (!ed.empty()) {
        cmd = ed + " \"" + p + "\"";
    } else {
        cmd = "xdg-open \"" + p + "\" 2>/dev/null || nano \"" + p + "\" 2>/dev/null || vim \"" + p + "\" 2>/dev/null || vi \"" + p + "\"";
    }
#endif
    (void)std::system(cmd.c_str());
}

void initConsole() {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
            #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
            #endif
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            (void)SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

} // namespace ulwai::platform
