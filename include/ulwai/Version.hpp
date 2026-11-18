/**
 * @file Version.hpp
 * @brief Centralized Application Version Information for ULWAI.
 *
 * PURPOSE & USAGE:
 *   Exposes the centralized version constants defined by the single source of truth (`VERSION`).
 *   Can be included anywhere in the codebase to obtain the semantic version string,
 *   major, minor, and patch integer components at compile-time or runtime.
 */

#pragma once

#include <string_view>

namespace ulwai {

#ifndef ULWAI_VERSION_STRING
#define ULWAI_VERSION_STRING "2.0.0"
#endif

#ifndef ULWAI_VERSION_MAJOR
#define ULWAI_VERSION_MAJOR 2
#define ULWAI_VERSION_MINOR 0
#define ULWAI_VERSION_PATCH 0
#endif

inline constexpr std::string_view kVersion = ULWAI_VERSION_STRING;
inline constexpr int kVersionMajor = ULWAI_VERSION_MAJOR;
inline constexpr int kVersionMinor = ULWAI_VERSION_MINOR;
inline constexpr int kVersionPatch = ULWAI_VERSION_PATCH;

[[nodiscard]] inline constexpr std::string_view getVersion() noexcept {
    return kVersion;
}

} // namespace ulwai
