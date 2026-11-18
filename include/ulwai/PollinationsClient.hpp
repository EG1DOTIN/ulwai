/**
 * @file PollinationsClient.hpp
 * @brief Default Free Cloud Generative AI Client Interface (Zero Keys Required).
 */

#pragma once

#include "ulwai/OpenAIClient.hpp"
#include <string>
#include <vector>

namespace ulwai {

class PollinationsClient : public OpenAIClient {
public:
    explicit PollinationsClient(
        std::string model = "openai",
        std::string endpoint = "https://text.pollinations.ai/openai",
        double temperature = 0.7,
        int max_tokens = 2048);

    [[nodiscard]] std::string name() const noexcept override {
        return "Pollinations (Free AI)";
    }

    [[nodiscard]] static std::vector<std::pair<std::string, std::string>> getAvailableModels();
};

} // namespace ulwai
