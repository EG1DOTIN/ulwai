/**
 * @file OllamaClient.hpp
 * @brief Local Offline Inference Client for Ollama & llama.cpp.
 */

#pragma once

#include "ulwai/IApiClient.hpp"
#include <string>

namespace ulwai {

class OllamaClient : public IApiClient {
public:
    explicit OllamaClient(std::string endpoint = "http://localhost:11434/api/chat",
                          std::string model = "llama3");

    [[nodiscard]] std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) override;

    [[nodiscard]] std::string name() const noexcept override {
        return "Ollama (Local)";
    }

private:
    std::string endpoint_;
    std::string model_;
};

} // namespace ulwai
