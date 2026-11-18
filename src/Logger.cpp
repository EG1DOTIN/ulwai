/**
 * @file Logger.cpp
 * @brief Thread-Safe Application Logging & Crash Reporting Implementation.
 */

#include "ulwai/Logger.hpp"
#include "ulwai/Platform.hpp"

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <thread>
#include <cstring>
#include <atomic>

#if defined(__linux__) || defined(__APPLE__)
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <execinfo.h>
#include <sys/types.h>
#endif

namespace ulwai {

namespace {

std::mutex g_logMutex;
std::ofstream g_logFile;
std::filesystem::path g_logDir;
std::filesystem::path g_appLogPath;
std::filesystem::path g_crashLogPath;
std::atomic<bool> g_initialized{false};

const char* levelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string getTimestampWithMillis() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream ss;
    ss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

#if defined(__linux__) || defined(__APPLE__)
// Signal-safe write helper
void safeWrite(int fd, const char* str) noexcept {
    if (fd < 0 || !str) return;
    size_t len = 0;
    while (str[len] != '\0') len++;
    ssize_t written = 0;
    while (written < static_cast<ssize_t>(len)) {
        ssize_t res = write(fd, str + written, len - written);
        if (res <= 0) break;
        written += res;
    }
}

void safeWriteNum(int fd, long num) noexcept {
    char buf[32];
    int i = 0;
    bool neg = (num < 0);
    if (neg) num = -num;
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num > 0 && i < 30);
    if (neg) buf[i++] = '-';
    for (int j = 0; j < i / 2; ++j) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    buf[i] = '\0';
    safeWrite(fd, buf);
}

void crashSignalHandler(int sig, siginfo_t* info, void* /*ucontext*/) {
    // Open crash.log directly with low-level syscalls (signal-safe)
    const char* path = g_crashLogPath.c_str();
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        fd = STDERR_FILENO;
    }

    safeWrite(fd, "\n=======================================================\n");
    safeWrite(fd, "  FATAL APPLICATION CRASH REPORT (ULWAI)\n");
    safeWrite(fd, "=======================================================\n");
    safeWrite(fd, "Received fatal signal: ");
    safeWriteNum(fd, sig);
    switch (sig) {
        case SIGSEGV: safeWrite(fd, " (SIGSEGV - Segmentation Fault)\n"); break;
        case SIGABRT: safeWrite(fd, " (SIGABRT - Abort)\n"); break;
        case SIGFPE:  safeWrite(fd, " (SIGFPE - Floating Point Exception)\n"); break;
        case SIGILL:  safeWrite(fd, " (SIGILL - Illegal Instruction)\n"); break;
        case SIGBUS:  safeWrite(fd, " (SIGBUS - Bus Error)\n"); break;
        default:      safeWrite(fd, " (Unknown Signal)\n"); break;
    }

    if (info) {
        safeWrite(fd, "Fault address: ");
        safeWriteNum(fd, reinterpret_cast<uintptr_t>(info->si_addr));
        safeWrite(fd, "\n");
    }

    safeWrite(fd, "\n--- Stack Backtrace ---\n");
    void* callstack[64];
    int frames = backtrace(callstack, 64);
    backtrace_symbols_fd(callstack, frames, fd);

    safeWrite(fd, "=======================================================\n\n");
    fsync(fd);
    if (fd != STDERR_FILENO) {
        close(fd);
    }

    // Also write notice to stderr
    safeWrite(STDERR_FILENO, "\n[FATAL CRASH] Segmentation fault or fatal signal detected!\n");
    safeWrite(STDERR_FILENO, "[FATAL CRASH] Detailed crash stack trace written to: ");
    safeWrite(STDERR_FILENO, path);
    safeWrite(STDERR_FILENO, "\n");

    // Restore default signal handler and re-raise to ensure proper exit code / coredump
    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, nullptr);
    raise(sig);
}
#endif

} // namespace

void initLogging(const std::filesystem::path& logDir) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_initialized) return;

    if (logDir.empty()) {
        g_logDir = platform::getConfigDirectory() / "logs";
    } else {
        g_logDir = logDir;
    }

    std::error_code ec;
    std::filesystem::create_directories(g_logDir, ec);

    g_appLogPath = g_logDir / "app.log";
    g_crashLogPath = g_logDir / "crash.log";

    g_logFile.open(g_appLogPath, std::ios::out | std::ios::app);
    g_initialized = true;

    // Log initialization banner
    if (g_logFile.is_open()) {
        g_logFile << "\n"
                  << "=================================================================\n"
                  << "  ULWAI AI Agent Logging Initialized: " << getTimestampWithMillis() << "\n"
                  << "  Log File: " << g_appLogPath.string() << "\n"
                  << "=================================================================\n";
        g_logFile.flush();
    }
}

void shutdownLogging() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_initialized) return;
    if (g_logFile.is_open()) {
        g_logFile << "[" << getTimestampWithMillis() << "] [INFO] ULWAI Logging Stopped Cleanly.\n";
        g_logFile.flush();
        g_logFile.close();
    }
    g_initialized = false;
}

void log(LogLevel level, const char* file, int line, const std::string& message) {
    if (!g_initialized) {
        initLogging();
    }

    std::string ts = getTimestampWithMillis();
    std::string filename = file ? std::filesystem::path(file).filename().string() : "unknown";

    std::ostringstream ss;
    ss << "[" << ts << "] [" << levelToString(level) << "] "
       << "[" << std::this_thread::get_id() << "] "
       << "[" << filename << ":" << line << "] "
       << message << "\n";

    std::string entry = ss.str();

    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile << entry;
        if (level == LogLevel::Error || level == LogLevel::Fatal) {
            g_logFile.flush();
        }
    }
}

void installCrashHandler() {
#if defined(__linux__) || defined(__APPLE__)
    if (!g_initialized) {
        initLogging();
    }

    struct sigaction sa{};
    sa.sa_sigaction = crashSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_RESETHAND;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);

    ULWAI_LOG_INFO("Native crash signal handlers installed (SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS)");
#endif
}

std::filesystem::path getAppLogPath() {
    return g_appLogPath;
}

std::filesystem::path getCrashLogPath() {
    return g_crashLogPath;
}

} // namespace ulwai
