/**
 * @file GrokClient.hpp
 * @brief xAI Grok API Client Interface.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Specialization of `OpenAIClient` configured for the xAI Grok REST API
 *   (`https://api.x.ai/v1/chat/completions`). Supports models like `grok-beta` and `grok-2`.
 */

#pragma once

#include "ulwai/OpenAIClient.hpp"

namespace ulwai {

class GrokClient : public OpenAIClient {
public:
    explicit GrokClient(std::string api_key,
                        std::string model = "grok-beta",
                        std::string endpoint = "https://api.x.ai/v1/chat/completions")
        : OpenAIClient(std::move(api_key),
                       std::move(model),
                       std::move(endpoint)) {}

    [[nodiscard]] std::string name() const noexcept override {
        return "xAI Grok";
    }
};

} // namespace ulwai
