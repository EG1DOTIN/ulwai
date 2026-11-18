/**
 * @file OpenRouterClient.cpp
 * @brief OpenRouter Multi-Model Aggregator API Client.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements `IApiClient` for the OpenRouter aggregation service (`https://openrouter.ai`).
 *   Provides a single unified interface to hundreds of top open-weights (Llama 3, Mistral,
 *   Qwen, Gemma, DeepSeek) and proprietary AI models (Claude, Gemini, GPT-4), including
 *   completely free tier models like `meta-llama/llama-3.1-8b-instruct:free`.
 *
 * IMPLEMENTATION DETAILS:
 *   - Attaches OpenRouter-specific ranking headers: `HTTP-Referer` and `X-Title: ULWAI AI Agent`.
 *   - Parses standard SSE streaming responses (`openRouterStreamCallback`).
 *   - Fallback error extraction with clean user messaging when quotas or invalid keys are detected.
 */

#include "ulwai/OpenRouterClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/CurlRaii.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>

namespace ulwai {

using json = nlohmann::json;

OpenRouterClient::OpenRouterClient(std::string api_key,
                                   std::string model,
                                   std::string endpoint,
                                   std::string referer)
    : api_key_(std::move(api_key)),
      model_(std::move(model)),
      endpoint_(std::move(endpoint)),
      referer_(std::move(referer))
{
    if (endpoint_.empty()) {
        endpoint_ = "https://openrouter.ai/api/v1/chat/completions";
    }
    if (model_.empty()) {
        model_ = "meta-llama/llama-3.1-8b-instruct:free";
    }
}

struct OpenRouterStreamCtx {
    StreamCallback onToken;
    std::string fullText;
    std::string buffer;
    bool hadSseTokens{false};
};

static size_t openRouterStreamCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    auto* ctx = static_cast<OpenRouterStreamCtx*>(userp);
    ctx->buffer.append(static_cast<char*>(contents), totalBytes);

    size_t startPos = 0;
    size_t nextPos = 0;
    std::string_view bufView(ctx->buffer);

    while ((nextPos = bufView.find('\n', startPos)) != std::string_view::npos) {
        std::string_view line = bufView.substr(startPos, nextPos - startPos);
        startPos = nextPos + 1;

        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        if (line.starts_with("data: ")) {
            std::string_view data = line.substr(6);
            if (data == "[DONE]") continue;
            try {
                auto j = json::parse(data);
                if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
                    const auto& choice = j["choices"][0];
                    if (choice.contains("delta") && choice["delta"].contains("content")) {
                        std::string delta = choice["delta"]["content"].get<std::string>();
                        ctx->fullText.append(delta);
                        ctx->hadSseTokens = true;
                        if (ctx->onToken) {
                            ctx->onToken(delta);
                        }
                    }
                }
            } catch (...) {}
        }
    }

    if (startPos > 0) {
        ctx->buffer.erase(0, startPos);
    }

    return totalBytes;
}

std::string OpenRouterClient::chat(
    const std::vector<ChatMessage>& messages,
    const std::string& system_prompt,
    StreamCallback onToken)
{
    if (api_key_.empty()) {
        std::string err = "OpenRouter API Key required. Set it in Settings or via --api-key";
        if (onToken) onToken(err);
        return err;
    }

    net::UniqueCurl curl(curl_easy_init());
    if (!curl) return "Error: Failed to initialize CURL";

    json req;
    req["model"] = model_;
    req["stream"] = (onToken != nullptr);

    json msgArray = json::array();
    if (!system_prompt.empty()) {
        json sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = system_prompt;
        msgArray.push_back(sysMsg);
    }

    for (const auto& msg : messages) {
        json m;
        m["role"] = std::string(roleToString(msg.role));
        m["content"] = msg.content;
        msgArray.push_back(m);
    }
    req["messages"] = msgArray;

    std::string payload = req.dump();
    std::string rawResponse;
    OpenRouterStreamCtx streamCtx;
    streamCtx.onToken = onToken;

    net::UniqueCurlSlist headers;
    net::appendHeader(headers, "Content-Type: application/json");
    std::string auth = "Authorization: Bearer " + api_key_;
    net::appendHeader(headers, auth.c_str());
    if (!referer_.empty()) {
        std::string ref = "HTTP-Referer: " + referer_;
        net::appendHeader(headers, ref.c_str());
    }
    net::appendHeader(headers, "X-Title: ULWAI-AI-Agent");

    curl_easy_setopt(curl.get(), CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 120L);

    if (onToken) {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, openRouterStreamCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &streamCtx);
    } else {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, utils::curlWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &rawResponse);
    }

    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        std::string err = std::string("Network Error (CURL): ") + curl_easy_strerror(res);
        if (onToken) onToken(err);
        return err;
    }

    if (onToken && streamCtx.hadSseTokens) {
        return streamCtx.fullText;
    }

    std::string toParse = onToken ? (streamCtx.buffer.empty() ? streamCtx.fullText : streamCtx.buffer) : rawResponse;
    try {
        auto j = json::parse(toParse);
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            const auto& choice = j["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                std::string full = choice["message"]["content"].get<std::string>();
                if (onToken && !streamCtx.hadSseTokens) onToken(full);
                return full;
            }
        }
        if (j.contains("error")) {
            std::string err = "OpenRouter Error: ";
            if (j["error"].is_object() && j["error"].contains("message")) {
                err += j["error"]["message"].get<std::string>();
            } else {
                err += j["error"].dump();
            }
            if (onToken) onToken(err);
            return err;
        }
    } catch (...) {}

    return toParse;
}

} // namespace ulwai
