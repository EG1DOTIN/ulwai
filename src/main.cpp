/**
 * @file main.cpp
 * @brief Unified Dual-Mode Entry Point for ULWAI AI Agent.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   This is the primary binary entry point for the standard `ulwai` and `ulw-ai-agent` executables.
 *   It provides automatic dual-mode execution (Desktop GUI vs. Terminal CLI) based on command-line
 *   flags, environment capabilities, and display availability:
 *
 * EXECUTION LOGIC & DISPATCH RULES:
 *   1. Initialize global configuration (`ulwai::Config`) by loading user settings from the OS-specific
 *      config directory (`~/.config/ulwai/config.json` on Linux/macOS, `%APPDATA%\ulwai\config.json` on Windows).
 *   2. Instantiate the core `ConversationManager` which coordinates multi-turn history, file attachments,
 *      and engine clients.
 *   3. Parse command-line arguments to determine execution mode:
 *      - If explicitly invoked with `--gui` (or launched with zero arguments in a graphical environment
 *        such as X11 / Wayland / Windows / macOS), initialize `QApplication` and launch `ulwai::ui::MainWindow`.
 *      - If CLI flags (`--cli`, `-c`, `-s`, `-f`, `-m`, or a direct prompt) are provided, or if running in
 *        a headless / SSH environment without an active display server, bypass Qt GUI initialization
 *        and dispatch directly to `ulwai::cli::CliRunner`.
 *
 * MEMORY & PERFORMANCE INVARIANTS:
 *   - Zero overhead for CLI mode: Qt event loop and widgets are completely unallocated when running in CLI.
 *   - Desktop GUI peak RAM usage is maintained strictly under 60-100 MB.
 *
 * THREAD SAFETY & LIFECYCLE:
 *   - Operates on the main application thread.
 *   - In GUI mode, background network queries run on `AiWorker` (a dedicated `QThread`), preventing UI freeze.
 */

#include "ulwai/Config.hpp"
#include "ulwai/ConversationManager.hpp"
#include "ulwai/cli/CliRunner.hpp"
#include "ulwai/Version.hpp"
#include "ulwai/Logger.hpp"

#include <iostream>
#include <cstdlib>
#include <string>

#ifdef ULWAI_ENABLE_GUI
#include <QApplication>
#include <QFont>
#include <QStringList>
#include <QtGlobal>
#include <QMessageLogContext>
#include "ulwai/ui/MainWindow.hpp"

static void qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    ulwai::LogLevel level = ulwai::LogLevel::Info;
    switch (type) {
        case QtDebugMsg:    level = ulwai::LogLevel::Debug; break;
        case QtInfoMsg:     level = ulwai::LogLevel::Info; break;
        case QtWarningMsg:  level = ulwai::LogLevel::Warning; break;
        case QtCriticalMsg: level = ulwai::LogLevel::Error; break;
        case QtFatalMsg:    level = ulwai::LogLevel::Fatal; break;
    }

    const char* file = context.file ? context.file : "Qt";
    int line = context.line;
    std::string text = msg.toStdString();

    ulwai::log(level, file, line, text);

    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        if (text.find("OpenType support missing") == std::string::npos) {
            std::cerr << "[Qt " << (level == ulwai::LogLevel::Warning ? "WARN" : "ERROR") << "] " << text << "\n";
        }
    }
}

static void installQtMessageHandler() {
    qInstallMessageHandler(qtMessageOutput);
    ULWAI_LOG_INFO("Qt message handler installed for font/layout/engine warnings");
}
#endif

int main(int argc, char* argv[]) {
    // 0. Initialize logging and install crash signal handlers (SIGSEGV, SIGABRT, etc.)
    ulwai::initLogging();
    ulwai::installCrashHandler();
    ULWAI_LOG_INFO("ULWAI v" + std::string(ulwai::getVersion()) + " initializing (process ID: " + std::to_string(getpid()) + ")");

    // 1. Load application configuration from standard OS path
    ulwai::Config config;
    (void)config.load();

    // 2. Initialize the shared ConversationManager (holds active chat state & history)
    ulwai::ConversationManager conversation(config);

    bool forceGui = false;
    bool hasCliFlags = false;

    // 3. Inspect command-line arguments to determine execution intent
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--gui") {
            forceGui = true;
        } else if (arg == "--cli" || arg == "-h" || arg == "--help" ||
                   arg == "-v" || arg == "--version" || arg == "-c" ||
                   arg == "--code" || arg == "-s" || arg == "--shell" ||
                   arg == "-f" || arg == "-m" || arg == "--system" ||
                   arg == "--config" || arg[0] != '-') {
            hasCliFlags = true;
        }
    }

#ifdef ULWAI_ENABLE_GUI
    // Check if an active display server exists on Linux (X11 or Wayland)
    bool hasDisplay = true;
#if defined(__linux__)
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if ((!display || *display == '\0') && (!wayland || *wayland == '\0')) {
        hasDisplay = false; // Headless Linux environment detected (e.g., SSH, Docker)
    }
#endif

    // If --gui is forced, or if launched without flags in a graphical environment -> launch GUI
    if ((forceGui || (!hasCliFlags && argc == 1)) && hasDisplay) {
        QApplication app(argc, argv);
        installQtMessageHandler();

        // Register multilingual font fallbacks so Devanagari/Hindi (script 11) is shaped properly on Linux
        QFont::insertSubstitutions("Segoe UI", QStringList{"Noto Sans Devanagari", "Noto Sans", "DejaVu Sans", "Liberation Sans", "sans-serif"});
        QFont::insertSubstitutions("Roboto", QStringList{"Noto Sans Devanagari", "Noto Sans", "DejaVu Sans", "Liberation Sans", "sans-serif"});

        ULWAI_LOG_INFO("Launching Desktop GUI mode");

        app.setApplicationName(QString::fromStdString(config.app_name));
        app.setApplicationVersion(QString::fromUtf8(ulwai::getVersion().data(), static_cast<int>(ulwai::getVersion().size())));

        ulwai::ui::MainWindow win(config, conversation);
        win.show();

        int res = app.exec();
        ulwai::shutdownLogging();
        return res;
    }
#endif

    // 4. Otherwise, dispatch to the fast terminal CLI runner
    ULWAI_LOG_INFO("Launching Terminal CLI mode");
    ulwai::cli::CliRunner cliRunner(config, conversation);
    int res = cliRunner.run(argc, argv);
    ulwai::shutdownLogging();
    return res;
}
