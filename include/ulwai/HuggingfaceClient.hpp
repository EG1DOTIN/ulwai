/**
 * @file HuggingfaceClient.hpp
 * @brief Hugging Face Serverless Inference API Client Interface.
 */

#pragma once

#include "ulwai/IApiClient.hpp"
#include <string>

namespace ulwai {

class HuggingfaceClient : public IApiClient {
public:
    HuggingfaceClient(std::string api_key,
                      std::string model = "mistralai/Mistral-7B-Instruct-v0.2",
                      std::string endpoint = "https://api-inference.huggingface.co/models/");

    [[nodiscard]] std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) override;

    [[nodiscard]] std::string name() const noexcept override {
        return "Hugging Face";
    }

private:
    std::string api_key_;
    std::string model_;
    std::string endpoint_;
};

} // namespace ulwai
