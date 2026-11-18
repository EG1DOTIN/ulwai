/**
 * @file HuggingfaceClient.cpp
 * @brief Hugging Face Serverless Inference API Client.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements `IApiClient` for the Hugging Face Serverless Inference API
 *   (`https://api-inference.huggingface.co/models/<model_id>`).
 *   Provides direct access to open models hosted on the Hugging Face Hub (e.g. `mistralai/Mistral-7B-Instruct-v0.2`).
 *
 * PROTOCOL DETAILS:
 *   - Authenticates via `Authorization: Bearer <HF_TOKEN>`.
 *   - Formats multi-turn chat history into standard Hugging Face conversation/prompt payload structures.
 *   - Handles Hugging Face JSON response formats (generated text array or conversation format).
 *   - Simulates streaming callback delivery for consistent user experience.
 */

#include "ulwai/HuggingfaceClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/CurlRaii.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>

namespace ulwai {

using json = nlohmann::json;

HuggingfaceClient::HuggingfaceClient(std::string api_key,
                                     std::string model,
                                     std::string endpoint)
    : api_key_(std::move(api_key)),
      model_(std::move(model)),
      endpoint_(std::move(endpoint))
{
    if (endpoint_.empty()) {
        endpoint_ = "https://api-inference.huggingface.co/models/";
    }
    if (model_.empty()) {
        model_ = "mistralai/Mistral-7B-Instruct-v0.2";
    }
}

std::string HuggingfaceClient::chat(
    const std::vector<ChatMessage>& messages,
    const std::string& system_prompt,
    StreamCallback onToken)
{
    if (api_key_.empty()) {
        std::string err = "Hugging Face API token required. Set it in Settings or via --api-key";
        if (onToken) onToken(err);
        return err;
    }

    net::UniqueCurl curl(curl_easy_init());
    if (!curl) return "Error: Failed to initialize CURL";

    std::string fullUrl = endpoint_;
    if (!fullUrl.empty() && fullUrl.back() != '/') fullUrl += '/';
    fullUrl += model_;

    std::string userPrompt;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (it->role == Role::User) {
            userPrompt = it->content;
            break;
        }
    }

    std::string formattedInput;
    if (!system_prompt.empty()) {
        formattedInput = "System: " + system_prompt + "\n\nUser: " + userPrompt;
    } else {
        formattedInput = userPrompt;
    }

    json req;
    req["inputs"] = formattedInput;
    req["parameters"]["max_new_tokens"] = 512;
    req["parameters"]["temperature"] = 0.7;

    std::string payload = req.dump();
    std::string rawResponse;

    net::UniqueCurlSlist headers;
    net::appendHeader(headers, "Content-Type: application/json");
    std::string auth = "Authorization: Bearer " + api_key_;
    net::appendHeader(headers, auth.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, utils::curlWriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &rawResponse);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        std::string err = std::string("Network Error (CURL): ") + curl_easy_strerror(res);
        if (onToken) onToken(err);
        return err;
    }

    std::string result;
    try {
        auto j = json::parse(rawResponse);
        if (j.is_array() && !j.empty() && j[0].contains("generated_text")) {
            result = j[0]["generated_text"].get<std::string>();
            // If the model echo-repeated the input prompt, remove it
            if (result.starts_with(formattedInput)) {
                result = result.substr(formattedInput.size());
            }
        } else if (j.is_object() && j.contains("error")) {
            result = "Hugging Face Error: " + j["error"].dump();
        } else {
            result = rawResponse;
        }
    } catch (...) {
        result = rawResponse;
    }

    if (onToken) {
        onToken(result);
    }
    return result;
}

} // namespace ulwai
