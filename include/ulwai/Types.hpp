/**
 * @file Types.hpp
 * @brief Core Domain Types, Enums & Data Structures for ULWAI.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Defines common value types used across both the GUI and CLI subsystems.
 *   Contains definitions for chat roles, messages, code blocks, sessions,
 *   and callback functional signatures.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <string_view>
#include <chrono>

namespace ulwai {

enum class Role {
    System,
    User,
    Assistant
};

[[nodiscard]] constexpr std::string_view roleToString(Role role) noexcept {
    switch (role) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
    }
    return "user";
}

[[nodiscard]] inline Role stringToRole(std::string_view str) noexcept {
    if (str == "system")    return Role::System;
    if (str == "assistant") return Role::Assistant;
    return Role::User;
}

struct CodeBlock {
    std::string language;
    std::string code;
    size_t index{0};
};

struct ChatMessage {
    Role role{Role::User};
    std::string content;
    std::string timestamp;
    std::vector<CodeBlock> code_blocks;
    bool is_shell_command{false};
    std::string shell_command_output;
    bool is_code_only{false};
};

struct ChatSession {
    std::string id;
    std::string title;
    std::string created_at;
    std::string updated_at;
    std::string engine;
    std::string model;
    std::vector<ChatMessage> messages;
};

using StreamCallback = std::function<void(std::string_view token)>;
using CompletionCallback = std::function<void(const std::string& full_text, const std::string& error)>;

enum class AgentMode {
    Chat,
    Code,
    Shell
};

/**
 * @brief Search and filtering options for session title querying.
 */
struct TitleSearchOptions {
    bool case_sensitive{false};  ///< Enforce exact case matching.
    bool match_whole_word{false}; ///< Match isolated word tokens bounded by non-alphanumeric chars.
    bool use_regex{false};        ///< Evaluate query as ECMAScript regular expression.
};

/**
 * @brief Result structure for online version update checks.
 */
struct UpdateCheckResult {
    bool success{false};           ///< Whether the remote check completed without network/HTTP errors.
    bool update_available{false};  ///< True if remote version is strictly newer than current version.
    std::string current_version;   ///< Local application version (e.g., "2.0.0").
    std::string latest_version;    ///< Discovered remote version (e.g., "2.1.0").
    std::string release_url;       ///< Direct URL to download release or view changelog.
    std::string error_message;     ///< Diagnostics message if network request failed.
};

} // namespace ulwai
