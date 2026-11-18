/**
 * @file Logger.hpp
 * @brief Thread-Safe Application Logging & Crash Reporting System for ULWAI.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Provides lightweight, thread-safe file and console logging along with native
 *   crash/signal interception (SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS).
 *   Integrates seamlessly with Qt's message handler to intercept font, layout,
 *   and runtime warnings into persistent log files.
 *
 * LOG LOCATIONS:
 *   - Application Log: ~/.config/ulwai/logs/app.log
 *   - Crash Report:   ~/.config/ulwai/logs/crash.log
 */

#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <sstream>

namespace ulwai {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * @brief Initialize logging infrastructure and create log directories.
 * @param logDir Optional custom directory; defaults to ~/.config/ulwai/logs.
 */
void initLogging(const std::filesystem::path& logDir = {});

/**
 * @brief Shuts down logging cleanly and flushes remaining buffers.
 */
void shutdownLogging();

/**
 * @brief Log a message with level, file location, and line number.
 */
void log(LogLevel level, const char* file, int line, const std::string& message);

/**
 * @brief Install native crash signal handler for SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS.
 *        Generates a stack backtrace and fatal diagnostic report into crash.log.
 */
void installCrashHandler();

/**
 * @brief Returns the absolute path to app.log.
 */
[[nodiscard]] std::filesystem::path getAppLogPath();

/**
 * @brief Returns the absolute path to crash.log.
 */
[[nodiscard]] std::filesystem::path getCrashLogPath();

} // namespace ulwai

// Convenience logging macros
#define ULWAI_LOG_DEBUG(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::ulwai::log(::ulwai::LogLevel::Debug, __FILE__, __LINE__, _oss.str()); \
} while (0)

#define ULWAI_LOG_INFO(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::ulwai::log(::ulwai::LogLevel::Info, __FILE__, __LINE__, _oss.str()); \
} while (0)

#define ULWAI_LOG_WARN(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::ulwai::log(::ulwai::LogLevel::Warning, __FILE__, __LINE__, _oss.str()); \
} while (0)

#define ULWAI_LOG_ERROR(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::ulwai::log(::ulwai::LogLevel::Error, __FILE__, __LINE__, _oss.str()); \
} while (0)

#define ULWAI_LOG_FATAL(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::ulwai::log(::ulwai::LogLevel::Fatal, __FILE__, __LINE__, _oss.str()); \
} while (0)
