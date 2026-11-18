/**
 * @file Utils.hpp
 * @brief Utility Functions Declarations: Markdown, HTML, Shell Parsing & libcurl Callbacks.
 */

#pragma once

#include "ulwai/Types.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <iosfwd>

namespace ulwai::utils {

[[nodiscard]] std::string urlEncode(std::string_view value);
[[nodiscard]] std::string escapeHtml(std::string_view text);

/**
 * @brief Streams HTML-escaped text directly into an output stream with zero heap allocations.
 * @param out Target output stream (e.g. std::ostringstream buffer).
 * @param text Raw input string view to escape.
 */
void escapeHtmlToStream(std::ostream& out, std::string_view text);

[[nodiscard]] std::string stripShellMarkdown(std::string_view input);
[[nodiscard]] std::string formatLanguageLabel(std::string_view lang);
[[nodiscard]] bool hasSignificantNaturalLanguageScript(std::string_view text);
[[nodiscard]] bool isLikelyPureCode(std::string_view text, std::string& detectedLang);
[[nodiscard]] size_t parseCopyBlockIndex(std::string_view url, bool& ok);
[[nodiscard]] std::vector<CodeBlock> extractCodeBlocks(std::string_view text);
[[nodiscard]] std::string markdownToHtml(std::string_view markdown, size_t start_code_index = 0, std::vector<CodeBlock>* outBlocks = nullptr, bool isCodeOnly = false);
[[nodiscard]] std::string readFileContents(const std::filesystem::path& path);
[[nodiscard]] std::string getCurrentTimestampIso();
[[nodiscard]] std::string getCurrentTimeReadable();

/**
 * @brief High-performance title search and filter evaluator for conversation history.
 *
 * Supports case-insensitive/sensitive substring search, word-boundary matching, and regex.
 * Executes in O(N) linear time with zero dynamic memory allocations for non-regex modes.
 *
 * @param title Conversation title to test against.
 * @param query Search query substring or expression.
 * @param options Configurable search flags (case_sensitive, match_whole_word, use_regex).
 * @return true if title satisfies query and filter constraints, false otherwise.
 */
[[nodiscard]] bool matchesTitle(std::string_view title, std::string_view query, const TitleSearchOptions& options = {}) noexcept;

void terminalTypingPrint(std::string_view text, unsigned int delayMs = 6);

size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

/**
 * @brief Compares two semantic version strings (e.g. "2.0.0" and "2.1.0" or "v2.0.0").
 * @return -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2.
 */
[[nodiscard]] int compareSemanticVersions(std::string_view v1, std::string_view v2) noexcept;

/**
 * @brief Checks if candidateVersion is strictly newer than currentVersion.
 */
[[nodiscard]] bool isNewerVersion(std::string_view currentVersion, std::string_view candidateVersion) noexcept;

/**
 * @brief Queries GitHub for the latest release version of ULWAI.
 *
 * Checks raw GitHub content first (no API rate limits), falling back to the GitHub Releases API.
 *
 * @param timeoutSeconds Connection and transfer timeout in seconds (default: 5s).
 * @return UpdateCheckResult with status, version strings, and release URL.
 */
[[nodiscard]] UpdateCheckResult checkForUpdatesOnline(long timeoutSeconds = 5);

} // namespace ulwai::utils
