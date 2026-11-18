/**
 * @file OpenRouterClient.hpp
 * @brief Multi-Model Gateway Aggregator Client Interface.
 */

#pragma once

#include "ulwai/IApiClient.hpp"
#include <string>

namespace ulwai {

class OpenRouterClient : public IApiClient {
public:
    OpenRouterClient(std::string api_key,
                     std::string model = "meta-llama/llama-3.1-8b-instruct:free",
                     std::string endpoint = "https://openrouter.ai/api/v1/chat/completions",
                     std::string referer = "https://github.com/ulw-ai-agent");

    [[nodiscard]] std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) override;

    [[nodiscard]] std::string name() const noexcept override {
        return "OpenRouter";
    }

private:
    std::string api_key_;
    std::string model_;
    std::string endpoint_;
    std::string referer_;
};

} // namespace ulwai
