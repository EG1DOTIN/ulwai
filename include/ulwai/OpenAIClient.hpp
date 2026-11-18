/**
 * @file OpenAIClient.hpp
 * @brief OpenAI & OpenAI-Compatible REST/SSE Streaming Engine Interface.
 */

#pragma once

#include "ulwai/IApiClient.hpp"
#include <string>

namespace ulwai {

class OpenAIClient : public IApiClient {
public:
    OpenAIClient(std::string api_key,
                 std::string model = "gpt-4o-mini",
                 std::string endpoint = "https://api.openai.com/v1/chat/completions",
                 double temperature = 0.7,
                 int max_tokens = 2048);

    [[nodiscard]] std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) override;

    [[nodiscard]] std::string name() const noexcept override {
        return "OpenAI-Compatible";
    }

private:
    std::string api_key_;
    std::string model_;
    std::string endpoint_;
    double temperature_{0.7};
    int max_tokens_{2048};
};

} // namespace ulwai
