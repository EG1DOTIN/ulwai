/**
 * @file ConversationManager.hpp
 * @brief Conversation Lifecycle Manager & Engine Factory Interface.
 *
 * Coordinates active session state, multi-turn history pruning,
 * JSON document persistence, and engine client factory instantiation.
 */

#pragma once

#include "ulwai/Types.hpp"
#include "ulwai/Config.hpp"
#include "ulwai/IApiClient.hpp"
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

namespace ulwai {

struct SessionMetadata {
    std::string id;
    std::string title;
    std::string updated_at;
    std::string engine;
};

class ConversationManager {
public:
    explicit ConversationManager(Config& config);

    [[nodiscard]] static std::unique_ptr<IApiClient> createClient(
        const std::string& engine,
        const Config& config);

    void newSession(const std::string& title = "New Conversation");
    [[nodiscard]] bool loadSession(const std::string& sessionId);
    [[nodiscard]] bool saveCurrentSession();
    [[nodiscard]] bool deleteSession(const std::string& sessionId);

    [[nodiscard]] std::vector<SessionMetadata> listSessions() const;

    void addMessage(Role role, std::string content, bool isShell = false, std::string shellOutput = "", bool isCode = false);
    void clearCurrentMessages();

    [[nodiscard]] const ChatSession& currentSession() const noexcept {
        return session_;
    }

    [[nodiscard]] ChatSession& currentSession() noexcept {
        return session_;
    }

    [[nodiscard]] std::vector<ChatMessage> getContextMessages(int limit = -1) const;

    [[nodiscard]] std::string exportToMarkdown() const;
    [[nodiscard]] std::string exportToHtml() const;
    [[nodiscard]] std::string exportAllConversationsJson() const;
    [[nodiscard]] size_t importConversationsJson(const std::string& jsonContent);
    [[nodiscard]] size_t deleteAllSessions();

private:
    Config& config_;
    ChatSession session_;
    std::filesystem::path history_dir_;
    mutable std::vector<SessionMetadata> cachedMetadata_; ///< In-memory metadata cache to prevent repeated O(N) disk scans.
    mutable bool metadataCacheValid_{false};              ///< True if cachedMetadata_ matches current disk and memory state.

    void ensureHistoryDirExists();
    void invalidateCache() const noexcept { metadataCacheValid_ = false; } ///< Invalidate metadata cache on any session mutation.
};

} // namespace ulwai
