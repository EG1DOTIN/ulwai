/**
 * @file ConversationManager.cpp
 * @brief Conversation Lifecycle, Session Persistence & Engine Factory Implementation.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Acts as the central business logic coordinator for ULWAI. It owns the active chat session,
 *   manages history persistence, generates polymorphic AI engine clients, and handles conversation
 *   import/export operations.
 *
 * RESPONSIBILITIES:
 *   1. Client Factory (`createClient`):
 *      - Implements the Factory Method pattern to instantiate polymorphic `IApiClient` instances
 *        for all 7 supported engines based on the engine string identifier.
 *   2. Active Session State:
 *      - Manages multi-turn conversation messages (`ChatMessage` structs containing role, content,
 *        timestamp, code blocks, model, and engine metadata).
 *      - Supports file attachments, prepending file contents with descriptive context blocks.
 *   3. Context Pruning & History Limit:
 *      - Enforces the configurable `history_limit` (default: 20 turns) to prevent unbounded memory
 *        growth and context overflow when querying LLM endpoints.
 *   4. Embedded Document Storage:
 *      - Persists chat sessions as discrete JSON files in `<ConfigDir>/history/<sessionId>.json`.
 *      - Automatically derives concise, human-friendly conversation titles from initial user prompts.
 *   5. Multi-Format Export Engine:
 *      - `exportSessionAsMarkdown`: Clean GitHub Flavored Markdown.
 *      - `exportSessionAsHtml`: Standalone HTML page with CSS styling.
 *      - `exportAllSessionsAsJson`: Comprehensive backup of all saved conversations.
 */

#include "ulwai/ConversationManager.hpp"
#include "ulwai/Platform.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/DuckDuckGoClient.hpp"
#include "ulwai/OllamaClient.hpp"
#include "ulwai/OpenAIClient.hpp"
#include "ulwai/OpenRouterClient.hpp"
#include "ulwai/PollinationsClient.hpp"
#include "ulwai/GrokClient.hpp"
#include "ulwai/HuggingfaceClient.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace ulwai {

using json = nlohmann::json;

ConversationManager::ConversationManager(Config& config)
    : config_(config), history_dir_(platform::getHistoryDirectory())
{
    ensureHistoryDirExists();
    newSession();
}

void ConversationManager::ensureHistoryDirExists() {
    std::error_code ec;
    std::filesystem::create_directories(history_dir_, ec);
}

std::unique_ptr<IApiClient> ConversationManager::createClient(
    const std::string& engine,
    const Config& config)
{
    std::string eng = engine.empty() ? config.default_engine : engine;

    if (eng == "pollinations" || eng == "free") {
        return std::make_unique<PollinationsClient>(
            config.pollinations.model,
            config.pollinations.endpoint,
            config.temperature,
            config.max_tokens
        );
    }
    if (eng == "duckduckgo" || eng == "duck") {
        return std::make_unique<DuckDuckGoClient>(config.duckduckgo.endpoint);
    }
    if (eng == "ollama") {
        return std::make_unique<OllamaClient>(config.ollama.endpoint, config.ollama.model);
    }
    if (eng == "openai") {
        return std::make_unique<OpenAIClient>(
            config.openai.api_key,
            config.openai.model,
            config.openai.endpoint,
            config.temperature,
            config.max_tokens
        );
    }
    if (eng == "openrouter") {
        return std::make_unique<OpenRouterClient>(
            config.openrouter.api_key,
            config.openrouter.model,
            config.openrouter.endpoint
        );
    }
    if (eng == "grok") {
        return std::make_unique<GrokClient>(
            config.grok.api_key,
            config.grok.model,
            config.grok.endpoint
        );
    }
    if (eng == "huggingface") {
        return std::make_unique<HuggingfaceClient>(
            config.huggingface.api_key,
            config.huggingface.model,
            config.huggingface.endpoint
        );
    }

    // Default to DuckDuckGo
    return std::make_unique<DuckDuckGoClient>(config.duckduckgo.endpoint);
}

void ConversationManager::newSession(const std::string& title) {
    invalidateCache();
    session_ = ChatSession{};
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "session_" << in_time_t << "_" << (rand() % 1000);

    session_.id = ss.str();
    session_.title = title;
    session_.created_at = utils::getCurrentTimeReadable();
    session_.updated_at = session_.created_at;
    session_.engine = config_.default_engine;
    session_.model = config_.resolveModel(session_.engine);
}

void ConversationManager::addMessage(Role role, std::string content, bool isShell, std::string shellOutput, bool isCode) {
    ChatMessage msg;
    msg.role = role;
    msg.content = std::move(content);
    msg.timestamp = utils::getCurrentTimeReadable();
    msg.is_shell_command = isShell;
    msg.shell_command_output = std::move(shellOutput);

    if (isCode) {
        std::string detected;
        if (utils::hasSignificantNaturalLanguageScript(msg.content) ||
            msg.content.find("### ") != std::string::npos ||
            msg.content.find("## ") != std::string::npos ||
            !utils::isLikelyPureCode(msg.content, detected)) {
            isCode = false;
        }
    }
    msg.is_code_only = isCode;

    if (role == Role::Assistant) {
        msg.code_blocks = utils::extractCodeBlocks(msg.content);
        for (auto& cb : msg.code_blocks) {
            cb.code.shrink_to_fit();
            cb.language.shrink_to_fit();
        }
        msg.code_blocks.shrink_to_fit();
    }
    msg.content.shrink_to_fit();

    // Auto-update title if it was default
    if (role == Role::User && session_.messages.empty()) {
        std::string snippet = msg.content.substr(0, std::min<size_t>(msg.content.size(), 30));
        auto newline_pos = snippet.find('\n');
        if (newline_pos != std::string::npos) {
            snippet = snippet.substr(0, newline_pos);
        }
        if (!snippet.empty()) {
            session_.title = snippet;
        }
    }

    session_.messages.push_back(std::move(msg));
    session_.updated_at = utils::getCurrentTimeReadable();
    (void)saveCurrentSession();
}

void ConversationManager::clearCurrentMessages() {
    session_.messages.clear();
    session_.updated_at = utils::getCurrentTimeReadable();
    (void)saveCurrentSession();
}

bool ConversationManager::saveCurrentSession() {
    invalidateCache();
    if (session_.id.empty()) return false;
    ensureHistoryDirExists();

    json j;
    j["id"] = session_.id;
    j["title"] = session_.title;
    j["created_at"] = session_.created_at;
    j["updated_at"] = session_.updated_at;
    j["engine"] = session_.engine;
    j["model"] = session_.model;

    json msgs = json::array();
    for (const auto& m : session_.messages) {
        json item;
        item["role"] = std::string(roleToString(m.role));
        item["content"] = m.content;
        item["timestamp"] = m.timestamp;
        item["is_shell"] = m.is_shell_command;
        item["shell_output"] = m.shell_command_output;
        item["is_code"] = m.is_code_only;
        msgs.push_back(item);
    }
    j["messages"] = msgs;

    std::filesystem::path filePath = history_dir_ / (session_.id + ".json");
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << j.dump(2);
    return true;
}

bool ConversationManager::loadSession(const std::string& sessionId) {
    invalidateCache();
    std::filesystem::path filePath = history_dir_ / (sessionId + ".json");
    if (!std::filesystem::exists(filePath)) return false;

    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    try {
        json j;
        in >> j;

        session_ = ChatSession{};
        session_.id = j.value("id", sessionId);
        session_.title = j.value("title", "Untitled Chat");
        session_.created_at = j.value("created_at", "");
        session_.updated_at = j.value("updated_at", "");
        session_.engine = j.value("engine", config_.default_engine);
        session_.model = j.value("model", "");

        if (j.contains("messages") && j["messages"].is_array()) {
            for (const auto& item : j["messages"]) {
                ChatMessage m;
                m.role = stringToRole(item.value("role", "user"));
                m.content = item.value("content", "");
                m.timestamp = item.value("timestamp", "");
                m.is_shell_command = item.value("is_shell", false);
                m.shell_command_output = item.value("shell_output", "");
                m.is_code_only = item.value("is_code", false);
                if (m.is_code_only) {
                    std::string detected;
                    if (utils::hasSignificantNaturalLanguageScript(m.content) ||
                        m.content.find("### ") != std::string::npos ||
                        m.content.find("## ") != std::string::npos ||
                        !utils::isLikelyPureCode(m.content, detected)) {
                        m.is_code_only = false;
                    }
                }
                if (m.role == Role::Assistant) {
                    m.code_blocks = utils::extractCodeBlocks(m.content);
                    for (auto& cb : m.code_blocks) {
                        cb.code.shrink_to_fit();
                        cb.language.shrink_to_fit();
                    }
                    m.code_blocks.shrink_to_fit();
                }
                m.content.shrink_to_fit();
                session_.messages.push_back(std::move(m));
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool ConversationManager::deleteSession(const std::string& sessionId) {
    invalidateCache();
    std::filesystem::path filePath = history_dir_ / (sessionId + ".json");
    std::error_code ec;
    // Safe deletion: move session file to trash archive directory instead of unrecoverable deletion
    if (std::filesystem::exists(filePath)) {
        std::filesystem::path trashDir = platform::getConfigDirectory() / "trash";
        std::filesystem::create_directories(trashDir, ec);
        std::filesystem::rename(filePath, trashDir / (sessionId + ".json"), ec);
    }
    if (session_.id == sessionId) {
        newSession();
    }
    return true;
}

std::vector<SessionMetadata> ConversationManager::listSessions() const {
    if (metadataCacheValid_) {
        return cachedMetadata_;
    }

    cachedMetadata_.clear();
    if (!std::filesystem::exists(history_dir_)) {
        metadataCacheValid_ = true;
        return cachedMetadata_;
    }

    for (const auto& entry : std::filesystem::directory_iterator(history_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream in(entry.path());
            if (in.is_open()) {
                try {
                    json j;
                    in >> j;
                    SessionMetadata meta;
                    meta.id = j.value("id", entry.path().stem().string());
                    meta.title = j.value("title", "Conversation");
                    meta.updated_at = j.value("updated_at", "");
                    meta.engine = j.value("engine", "");
                    cachedMetadata_.push_back(std::move(meta));
                } catch (...) {}
            }
        }
    }

    bool currentFound = false;
    for (const auto& m : cachedMetadata_) {
        if (m.id == session_.id) {
            currentFound = true;
            break;
        }
    }
    if (!currentFound && !session_.id.empty()) {
        SessionMetadata cur;
        cur.id = session_.id;
        cur.title = session_.title.empty() ? "New Conversation" : session_.title;
        cur.updated_at = session_.updated_at.empty() ? utils::getCurrentTimeReadable() : session_.updated_at;
        cur.engine = session_.engine;
        cachedMetadata_.push_back(cur);
    }

    // Sort by updated_at descending (latest to oldest)
    std::sort(cachedMetadata_.begin(), cachedMetadata_.end(), [](const SessionMetadata& a, const SessionMetadata& b) {
        if (a.updated_at != b.updated_at) return a.updated_at > b.updated_at;
        return a.id > b.id;
    });

    metadataCacheValid_ = true;
    return cachedMetadata_;
}

std::vector<ChatMessage> ConversationManager::getContextMessages(int limit) const {
    int maxMessages = (limit >= 0) ? limit : config_.history_limit;
    if (maxMessages <= 0 || static_cast<int>(session_.messages.size()) <= maxMessages) {
        return session_.messages;
    }

    std::vector<ChatMessage> sub;
    size_t start = session_.messages.size() - static_cast<size_t>(maxMessages);
    sub.reserve(static_cast<size_t>(maxMessages));
    sub.insert(sub.end(), session_.messages.begin() + start, session_.messages.end());
    return sub;
}

std::string ConversationManager::exportToMarkdown() const {
    std::ostringstream ss;
    ss << "# " << session_.title << "\n\n";
    ss << "- **Session ID:** " << session_.id << "\n";
    ss << "- **Date:** " << session_.created_at << "\n";
    ss << "- **Engine:** " << session_.engine << " (" << session_.model << ")\n\n---\n\n";

    for (const auto& m : session_.messages) {
        if (m.role == Role::User) {
            ss << "### 👤 User (" << m.timestamp << ")\n\n" << m.content << "\n\n";
        } else if (m.role == Role::Assistant) {
            ss << "### 🤖 Assistant (" << m.timestamp << ")\n\n" << m.content << "\n\n";
            if (m.is_shell_command && !m.shell_command_output.empty()) {
                ss << "```console\n" << m.shell_command_output << "\n```\n\n";
            }
        } else {
            ss << "### ⚙️ System\n\n" << m.content << "\n\n";
        }
        ss << "---\n\n";
    }
    return ss.str();
}

std::string ConversationManager::exportToHtml() const {
    std::ostringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>"
       << utils::escapeHtml(session_.title) << "</title>";
    ss << "<style>"
          "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #18181b; color: #f4f4f5; max-width: 800px; margin: 40px auto; padding: 20px; line-height: 1.6; }\n"
          ".msg { margin-bottom: 24px; padding: 16px 20px; border-radius: 12px; }\n"
          ".user { background: #27272a; border-left: 4px solid #10a37f; }\n"
          ".assistant { background: #1f1f23; border-left: 4px solid #3b82f6; }\n"
          ".author { font-size: 0.85em; font-weight: bold; margin-bottom: 8px; color: #a1a1aa; }\n"
          "pre { background: #09090b; padding: 12px; border-radius: 6px; overflow-x: auto; color: #38bdf8; }\n"
          "code { font-family: 'Fira Code', monospace; }\n"
          "</style></head><body>";

    ss << "<h1>" << utils::escapeHtml(session_.title) << "</h1>";
    ss << "<p><em>Created: " << session_.created_at << " | Engine: " << session_.engine << "</em></p><hr/>";

    for (const auto& m : session_.messages) {
        std::string roleClass = (m.role == Role::User) ? "user" : "assistant";
        std::string roleLabel = (m.role == Role::User) ? "User" : "Assistant";

        ss << "<div class=\"msg " << roleClass << "\">";
        ss << "<div class=\"author\">" << roleLabel << " • " << m.timestamp << "</div>";
        ss << utils::markdownToHtml(m.content);
        if (m.is_shell_command && !m.shell_command_output.empty()) {
            ss << "<pre><code>" << utils::escapeHtml(m.shell_command_output) << "</code></pre>";
        }
        ss << "</div>";
    }

    ss << "</body></html>";
    return ss.str();
}

std::string ConversationManager::exportAllConversationsJson() const {
    json root = json::array();
    if (!std::filesystem::exists(history_dir_)) return root.dump(2);

    for (const auto& entry : std::filesystem::directory_iterator(history_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream in(entry.path());
            if (in.is_open()) {
                try {
                    json j;
                    in >> j;

                    json conv;
                    std::string id = j.value("id", entry.path().stem().string());
                    std::string title = j.value("title", "Conversation");
                    std::string createdAt = j.value("created_at", "");
                    std::string updatedAt = j.value("updated_at", "");

                    conv["id"] = id;
                    conv["conversationId"] = id;
                    conv["title"] = title;
                    conv["create_time"] = createdAt;
                    conv["update_time"] = updatedAt;

                    json msgs = json::array();
                    json mapping = json::object();
                    std::string prevNodeId = "root";

                    // Root mapping node for ChatGPT format compatibility
                    json rootNode;
                    rootNode["id"] = "root";
                    rootNode["message"] = nullptr;
                    rootNode["parent"] = nullptr;
                    rootNode["children"] = json::array();
                    mapping["root"] = rootNode;

                    if (j.contains("messages") && j["messages"].is_array()) {
                        size_t nodeIdx = 0;
                        for (const auto& item : j["messages"]) {
                            msgs.push_back(item);

                            std::string nodeId = "msg_" + std::to_string(nodeIdx++);
                            mapping[prevNodeId]["children"].push_back(nodeId);

                            json node;
                            node["id"] = nodeId;
                            node["parent"] = prevNodeId;
                            node["children"] = json::array();

                            json msgObj;
                            msgObj["id"] = nodeId;
                            msgObj["author"] = {{"role", item.value("role", "user")}};
                            msgObj["content"] = {
                                {"content_type", "text"},
                                {"parts", json::array({item.value("content", "")})}
                            };
                            msgObj["create_time"] = item.value("timestamp", "");
                            node["message"] = msgObj;

                            mapping[nodeId] = node;
                            prevNodeId = nodeId;
                        }
                    }

                    conv["messages"] = msgs;
                    conv["mapping"] = mapping;
                    conv["current_node"] = prevNodeId;

                    root.push_back(conv);
                } catch (...) {}
            }
        }
    }

    return root.dump(2);
}

size_t ConversationManager::importConversationsJson(const std::string& jsonContent) {
    if (jsonContent.empty()) return 0;
    invalidateCache();
    ensureHistoryDirExists();

    json root = json::parse(jsonContent, nullptr, false);
    if (root.is_discarded()) return 0;

    json convArray = json::array();
    if (root.is_array()) {
        convArray = root;
    } else if (root.is_object()) {
        convArray.push_back(root);
    } else {
        return 0;
    }

    size_t importedCount = 0;
    std::string lastImportedId;

    for (const auto& conv : convArray) {
        if (!conv.is_object()) continue;

        ChatSession imported;
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        auto parseTimeString = [](const json& parent, const std::string& key) -> std::string {
            if (!parent.contains(key)) return "";
            const auto& v = parent[key];
            if (v.is_string()) return v.get<std::string>();
            if (v.is_number()) {
                double ct = v.get<double>();
                time_t t = static_cast<time_t>(ct);
                struct tm tm_buf;
                localtime_r(&t, &tm_buf);
                char buf[64];
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
                return std::string(buf);
            }
            return "";
        };

        std::string rawId;
        if (conv.contains("id") && conv["id"].is_string()) {
            rawId = conv["id"].get<std::string>();
        } else if (conv.contains("conversationId") && conv["conversationId"].is_string()) {
            rawId = conv["conversationId"].get<std::string>();
        }

        if (rawId.empty()) {
            std::stringstream ss;
            ss << "session_import_" << in_time_t << "_" << importedCount;
            rawId = ss.str();
        } else {
            rawId = "import_" + rawId;
        }

        imported.id = rawId;
        imported.title = (conv.contains("title") && conv["title"].is_string()) ? conv["title"].get<std::string>() : "Imported Conversation";

        std::string createdAt = parseTimeString(conv, "created_at");
        if (createdAt.empty()) createdAt = parseTimeString(conv, "create_time");
        if (createdAt.empty()) createdAt = utils::getCurrentTimeReadable();
        imported.created_at = createdAt;

        std::string updatedAt = parseTimeString(conv, "updated_at");
        if (updatedAt.empty()) updatedAt = parseTimeString(conv, "update_time");
        if (updatedAt.empty()) updatedAt = imported.created_at;
        imported.updated_at = updatedAt;

        imported.engine = config_.default_engine;
        imported.model = config_.resolveModel(imported.engine);

        // Case A: ChatGPT mapping tree format
        if (conv.contains("mapping") && conv["mapping"].is_object()) {
            const auto& mapping = conv["mapping"];
            std::vector<json> messageNodes;

            if (conv.contains("current_node") && conv["current_node"].is_string()) {
                std::string curr = conv["current_node"].get<std::string>();
                while (!curr.empty() && mapping.contains(curr)) {
                    auto node = mapping[curr];
                    if (node.contains("message") && !node["message"].is_null()) {
                        messageNodes.push_back(node["message"]);
                    }
                    if (node.contains("parent") && node["parent"].is_string()) {
                        curr = node["parent"].get<std::string>();
                    } else {
                        break;
                    }
                }
                std::reverse(messageNodes.begin(), messageNodes.end());
            }

            if (messageNodes.empty()) {
                for (auto it = mapping.begin(); it != mapping.end(); ++it) {
                    if (it.value().contains("message") && !it.value()["message"].is_null()) {
                        messageNodes.push_back(it.value()["message"]);
                    }
                }
                std::sort(messageNodes.begin(), messageNodes.end(), [](const json& a, const json& b) {
                    return a.value("create_time", 0.0) < b.value("create_time", 0.0);
                });
            }

            for (const auto& msgNode : messageNodes) {
                std::string roleStr = "user";
                if (msgNode.contains("author") && msgNode["author"].contains("role") && msgNode["author"]["role"].is_string()) {
                    roleStr = msgNode["author"]["role"].get<std::string>();
                }
                if (roleStr == "system") continue;

                std::string textContent;
                if (msgNode.contains("content") && msgNode["content"].is_object()) {
                    const auto& content = msgNode["content"];
                    if (content.contains("parts") && content["parts"].is_array()) {
                        for (const auto& part : content["parts"]) {
                            if (part.is_string()) {
                                textContent.append(part.get<std::string>());
                            } else if (part.is_object()) {
                                // Picture / Video / File attachment fallback link
                                std::string url = (part.contains("url") && part["url"].is_string()) ? part["url"].get<std::string>() :
                                                  (part.contains("asset_pointer") && part["asset_pointer"].is_string() ? part["asset_pointer"].get<std::string>() : "");
                                std::string name = (part.contains("name") && part["name"].is_string()) ? part["name"].get<std::string>() :
                                                   (part.contains("content_type") && part["content_type"].is_string() ? part["content_type"].get<std::string>() : "Attachment");
                                if (!url.empty()) {
                                    textContent.append("\n[Attachment: " + name + "](" + url + ")\n");
                                } else {
                                    textContent.append("\n[Attachment: " + name + "]\n");
                                }
                            }
                        }
                    } else if (content.contains("text") && content["text"].is_string()) {
                        textContent = content["text"].get<std::string>();
                    }
                }

                if (textContent.empty()) continue;

                std::string ts = parseTimeString(msgNode, "create_time");
                if (ts.empty()) ts = utils::getCurrentTimeReadable();

                ChatMessage m;
                m.role = stringToRole(roleStr);
                m.content = textContent;
                m.timestamp = ts;
                imported.messages.push_back(std::move(m));
            }
        }
        // Case B: LibreChat / ULWAI flat messages array format
        else if (conv.contains("messages") && conv["messages"].is_array()) {
            for (const auto& item : conv["messages"]) {
                std::string roleStr = item.value("role", item.value("sender", "user"));
                std::transform(roleStr.begin(), roleStr.end(), roleStr.begin(), ::tolower);
                if (roleStr == "system") continue;

                std::string text;
                if (item.contains("text") && item["text"].is_string()) {
                    text = item["text"].get<std::string>();
                } else if (item.contains("content") && item["content"].is_string()) {
                    text = item["content"].get<std::string>();
                }
                if (text.empty()) continue;

                std::string ts = parseTimeString(item, "createdAt");
                if (ts.empty()) ts = parseTimeString(item, "timestamp");
                if (ts.empty()) ts = utils::getCurrentTimeReadable();

                ChatMessage m;
                m.role = stringToRole(roleStr);
                m.content = text;
                m.timestamp = ts;
                imported.messages.push_back(std::move(m));
            }
        }

        if (!imported.messages.empty()) {
            // Write session file
            json j;
            j["id"] = imported.id;
            j["title"] = imported.title;
            j["created_at"] = imported.created_at;
            j["updated_at"] = imported.updated_at;
            j["engine"] = imported.engine;
            j["model"] = imported.model;

            json msgs = json::array();
            for (const auto& msg : imported.messages) {
                json m;
                m["role"] = std::string(roleToString(msg.role));
                m["content"] = msg.content;
                m["timestamp"] = msg.timestamp;
                m["is_shell"] = msg.is_shell_command;
                m["shell_output"] = msg.shell_command_output;
                m["is_code"] = msg.is_code_only;
                msgs.push_back(m);
            }
            j["messages"] = msgs;

            std::filesystem::path filePath = history_dir_ / (imported.id + ".json");
            std::ofstream out(filePath);
            if (out.is_open()) {
                out << j.dump(2);
                lastImportedId = imported.id;
                importedCount++;
            }
        }
    }

    if (importedCount > 0 && !lastImportedId.empty()) {
        (void)loadSession(lastImportedId);
    }

    return importedCount;
}

size_t ConversationManager::deleteAllSessions() {
    invalidateCache();
    size_t count = 0;
    ensureHistoryDirExists();
    std::filesystem::path trashDir = platform::getConfigDirectory() / "trash";
    std::error_code ec;
    std::filesystem::create_directories(trashDir, ec);

    for (const auto& entry : std::filesystem::directory_iterator(history_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::filesystem::rename(entry.path(), trashDir / entry.path().filename(), ec);
            count++;
        }
    }

    newSession();
    return count;
}

} // namespace ulwai
