/**
 * @file PollinationsClient.cpp
 * @brief Default Free Cloud Generative AI Engine (Pollinations.ai).
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Specialization of `OpenAIClient` configured for the public Pollinations.ai API.
 *   Serves as ULWAI's default out-of-the-box engine because it requires ZERO API keys,
 *   zero account creation, and zero subscription costs, providing immediate conversational
 *   AI and code generation capabilities on first install.
 *
 * ARCHITECTURE & INHERITANCE:
 *   - Inherits full streaming (`SSE` - Server-Sent Events) and non-streaming capabilities from
 *     `OpenAIClient` because Pollinations implements the OpenAI Chat Completions protocol (`/v1/chat/completions`).
 *   - Overrides model catalog via `getAvailableModels()` to expose active free community models:
 *     `openai` (GPT-OSS 20B reasoning), `openai-fast` (low latency), and `gpt-oss`.
 */

#include "ulwai/PollinationsClient.hpp"

namespace ulwai {

PollinationsClient::PollinationsClient(
    std::string model,
    std::string endpoint,
    double temperature,
    int max_tokens)
    : OpenAIClient(
          /*api_key=*/"",
          model.empty() ? "openai" : std::move(model),
          endpoint.empty() ? "https://text.pollinations.ai/openai" : std::move(endpoint),
          temperature,
          max_tokens) {}

std::vector<std::pair<std::string, std::string>> PollinationsClient::getAvailableModels() {
    return {
        {"openai", "OpenAI (GPT-OSS 20B Reasoning) [Default]"},
        {"openai-fast", "OpenAI Fast (Low Latency)"},
        {"gpt-oss", "GPT-OSS 20B (Open-Source Architecture)"}
    };
}

} // namespace ulwai
