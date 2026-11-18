/**
 * @file OpenAIClient.cpp
 * @brief OpenAI & OpenAI-Compatible REST/SSE Streaming Engine Client.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements `IApiClient` for the industry-standard OpenAI Chat Completions specification.
 *   Serves as the foundation for querying:
 *     - Official OpenAI endpoints (`gpt-4o-mini`, `gpt-4o`, `gpt-3.5-turbo`).
 *     - Local OpenAI-compatible servers (LM Studio, vLLM, llama.cpp `server`, LocalAI, Ollama `/v1`).
 *     - Cloud OpenAI-compatible providers (Groq, DeepSeek, Together.ai, Mistral, xAI Grok).
 *     - Subclasses: `PollinationsClient` and `OpenRouterClient`.
 *
 * STREAMING & SSE ENGINE:
 *   - Implements a robust line-buffered parser (`openAiStreamCallback`) for HTTP Server-Sent Events (`text/event-stream`).
 *   - Buffers partial incoming chunks from libcurl, splits on newlines (`\n` and `\r\n`), and extracts:
 *       `data: {"choices":[{"delta":{"content":"..."}}]}`
 *   - Invokes `StreamCallback` per token delta, enabling smooth real-time token rendering in the UI/terminal.
 *   - Gracefully detects `data: [DONE]` and HTTP error payloads (such as invalid API key or rate limit JSON).
 */

#include "ulwai/OpenAIClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/CurlRaii.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>

namespace ulwai {

using json = nlohmann::json;

OpenAIClient::OpenAIClient(std::string api_key,
                           std::string model,
                           std::string endpoint,
                           double temperature,
                           int max_tokens)
    : api_key_(std::move(api_key)),
      model_(std::move(model)),
      endpoint_(std::move(endpoint)),
      temperature_(temperature),
      max_tokens_(max_tokens)
{
    if (endpoint_.empty()) {
        endpoint_ = "https://api.openai.com/v1/chat/completions";
    }
}

struct StreamContext {
    StreamCallback onToken;
    std::string fullAccumulated;
    std::string buffer;
    bool hadSseTokens{false};
};

static size_t openAiStreamCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    auto* ctx = static_cast<StreamContext*>(userp);
    ctx->buffer.append(static_cast<char*>(contents), totalBytes);

    // Linear scanning using string_view without quadratic buffer copies
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
            if (data == "[DONE]") {
                continue;
            }
            try {
                auto j = json::parse(data);
                if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
                    const auto& choice = j["choices"][0];
                    if (choice.contains("delta") && choice["delta"].contains("content")) {
                        std::string delta = choice["delta"]["content"].get<std::string>();
                        ctx->fullAccumulated.append(delta);
                        ctx->hadSseTokens = true;
                        if (ctx->onToken) {
                            ctx->onToken(delta);
                        }
                    }
                }
            } catch (...) {
                // Ignore SSE heartbeat or non-json chunk
            }
        }
    }

    if (startPos > 0) {
        ctx->buffer.erase(0, startPos);
    }

    return totalBytes;
}

std::string OpenAIClient::chat(
    const std::vector<ChatMessage>& messages,
    const std::string& system_prompt,
    StreamCallback onToken)
{
    net::UniqueCurl curl(curl_easy_init());
    if (!curl) return "Error: Failed to initialize CURL";

    json req;
    req["model"] = model_.empty() ? "gpt-4o-mini" : model_;
    req["temperature"] = temperature_;
    if (max_tokens_ > 0) req["max_tokens"] = max_tokens_;
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
    StreamContext streamCtx;
    streamCtx.onToken = onToken;

    net::UniqueCurlSlist headers;
    net::appendHeader(headers, "Content-Type: application/json");
    if (!api_key_.empty()) {
        std::string auth = "Authorization: Bearer " + api_key_;
        net::appendHeader(headers, auth.c_str());
    }

    curl_easy_setopt(curl.get(), CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 120L);

    if (onToken) {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, openAiStreamCallback);
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
        return streamCtx.fullAccumulated;
    }

    // Either non-streaming or server returned an error body / non-SSE json
    std::string toParse = onToken ? (streamCtx.buffer.empty() ? streamCtx.fullAccumulated : streamCtx.buffer) : rawResponse;

    try {
        auto j = json::parse(toParse);
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            const auto& choice = j["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                std::string fullContent = choice["message"]["content"].get<std::string>();
                if (onToken && !streamCtx.hadSseTokens) onToken(fullContent);
                return fullContent;
            }
        }
        if (j.contains("error")) {
            std::string errDesc = "API Error: ";
            if (j["error"].is_object() && j["error"].contains("message")) {
                errDesc += j["error"]["message"].get<std::string>();
            } else {
                errDesc += j["error"].dump();
            }
            if (onToken) onToken(errDesc);
            return errDesc;
        }
    } catch (...) {
        // Return raw content if not JSON
    }

    return toParse;
}

} // namespace ulwai
