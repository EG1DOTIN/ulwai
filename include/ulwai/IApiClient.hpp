/**
 * @file IApiClient.hpp
 * @brief Abstract Base Interface for all AI Model & Search Providers.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Defines the polymorphic contract for executing chat completions and streaming tokens.
 *   All 7 AI backends (Pollinations, DuckDuckGo, Ollama, OpenAI, OpenRouter, Grok, Hugging Face)
 *   implement this interface.
 *
 * DESIGN PATTERNS:
 *   - Strategy Pattern: `ConversationManager` and `AiWorker` operate entirely on `IApiClient`
 *     pointers without knowing the underlying transport, protocol, or provider specifics.
 *   - Observer / Callback: Token streaming is handled via optional `StreamCallback` lambdas.
 */

#pragma once

#include "ulwai/Types.hpp"
#include <string>
#include <vector>

namespace ulwai {

class IApiClient {
public:
    virtual ~IApiClient() = default;

    [[nodiscard]] virtual std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) = 0;

    [[nodiscard]] std::string chatSingle(
        const std::string& prompt,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr)
    {
        std::vector<ChatMessage> msgs;
        msgs.push_back({Role::User, prompt, "", {}, false, ""});
        return chat(msgs, system_prompt, onToken);
    }

    [[nodiscard]] virtual std::string name() const noexcept = 0;
};

} // namespace ulwai
