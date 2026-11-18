/**
 * @file OllamaClient.cpp
 * @brief Local Offline LLM Client for Ollama & llama.cpp.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements `IApiClient` for Ollama's native `/api/chat` REST endpoint.
 *   Enables 100% offline, privacy-first inference running on the user's local CPU or GPU.
 *   No telemetry, no external internet connection required, zero API keys.
 *
 * HOW IT WORKS:
 *   1. REST Request:
 *      - Issues HTTP POST with JSON body: `{"model":"<name>", "messages":[...], "stream":true/false}`.
 *   2. Streaming Protocol (NDJSON):
 *      - Unlike OpenAI's SSE `data: ` format, Ollama streams Newline-Delimited JSON (NDJSON) chunks:
 *        `{"model":"llama3", "created_at":"...", "message":{"role":"assistant","content":"..."}, "done":false}`.
 *      - The stream callback (`ollamaStreamCallback`) buffers partial chunks, splits on `\n`,
 *        parses each individual JSON object, and fires `onToken(delta)`.
 *      - Detects the final packet when `done == true`.
 */

#include "ulwai/OllamaClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/CurlRaii.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>

namespace ulwai {

using json = nlohmann::json;

OllamaClient::OllamaClient(std::string endpoint, std::string model)
    : endpoint_(std::move(endpoint)), model_(std::move(model))
{
    if (endpoint_.empty()) {
        endpoint_ = "http://localhost:11434/api/chat";
    }
    if (model_.empty()) {
        model_ = "llama3";
    }
}

struct OllamaStreamCtx {
    StreamCallback onToken;
    std::string fullText;
    std::string buffer;
};

static size_t ollamaStreamCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    auto* ctx = static_cast<OllamaStreamCtx*>(userp);
    ctx->buffer.append(static_cast<char*>(contents), totalBytes);

    size_t startPos = 0;
    size_t nextPos = 0;
    std::string_view bufView(ctx->buffer);

    while ((nextPos = bufView.find('\n', startPos)) != std::string_view::npos) {
        std::string_view line = bufView.substr(startPos, nextPos - startPos);
        startPos = nextPos + 1;

        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        try {
            auto j = json::parse(line);
            if (j.contains("message") && j["message"].contains("content")) {
                std::string delta = j["message"]["content"].get<std::string>();
                ctx->fullText.append(delta);
                if (ctx->onToken) {
                    ctx->onToken(delta);
                }
            }
        } catch (...) {
            // Incomplete JSON line or parse error
        }
    }

    if (startPos > 0) {
        ctx->buffer.erase(0, startPos);
    }
    return totalBytes;
}

std::string OllamaClient::chat(
    const std::vector<ChatMessage>& messages,
    const std::string& system_prompt,
    StreamCallback onToken)
{
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
    OllamaStreamCtx streamCtx;
    streamCtx.onToken = onToken;

    net::UniqueCurlSlist headers;
    net::appendHeader(headers, "Content-Type: application/json");

    curl_easy_setopt(curl.get(), CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 300L);

    if (onToken) {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, ollamaStreamCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &streamCtx);
    } else {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, utils::curlWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &rawResponse);
    }

    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        std::string err = std::string("Error connecting to Ollama: ") + curl_easy_strerror(res) +
                          "\nMake sure Ollama is running (e.g. 'ollama serve' or 'ollama run " + model_ + "')";
        if (onToken) onToken(err);
        return err;
    }

    if (onToken) {
        return streamCtx.fullText;
    }

    try {
        auto j = json::parse(rawResponse);
        if (j.contains("message") && j["message"].contains("content")) {
            return j["message"]["content"].get<std::string>();
        }
        if (j.contains("error")) {
            return "Ollama Error: " + j["error"].get<std::string>();
        }
    } catch (...) {
        // Fallback
    }

    return rawResponse;
}

} // namespace ulwai
