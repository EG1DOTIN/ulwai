/**
 * @file DuckDuckGoClient.hpp
 * @brief Free Instant Search, Wikipedia Abstract & Math Client Interface.
 */

#pragma once

#include "ulwai/IApiClient.hpp"
#include <string>

namespace ulwai {

class DuckDuckGoClient : public IApiClient {
public:
    explicit DuckDuckGoClient(std::string endpoint = "https://api.duckduckgo.com/");

    [[nodiscard]] std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) override;

    [[nodiscard]] std::string name() const noexcept override {
        return "DuckDuckGo";
    }

private:
    std::string endpoint_;
};

} // namespace ulwai
