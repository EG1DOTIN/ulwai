/**
 * @file Platform.hpp
 * @brief Cross-Platform OS Abstraction Layer Declarations.
 *
 * Provides platform-independent directory paths, subprocess execution pipes,
 * system clipboard access, console UTF-8 initialization, and text editor launching.
 */

#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace ulwai::platform {

[[nodiscard]] std::filesystem::path getHomeDirectory();
[[nodiscard]] std::filesystem::path getConfigDirectory();
[[nodiscard]] std::filesystem::path getHistoryDirectory();
[[nodiscard]] std::filesystem::path getDefaultConfigFilePath();

[[nodiscard]] int executeCommand(const std::string& command, std::string& output);
void copyToClipboard(const std::string& text);
void openFileInDefaultEditor(const std::filesystem::path& path);
void initConsole();

} // namespace ulwai::platform
