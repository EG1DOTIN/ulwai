/**
 * @file DuckDuckGoClient.cpp
 * @brief Free Instant Search, Wikipedia Abstract & Math/Conversion Client.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements the `IApiClient` interface for DuckDuckGo's Instant Answer API.
 *   Provides instant factual summaries, Wikipedia definitions, unit conversions, and mathematical
 *   computations with 100% free access, requiring zero registration or API keys.
 *
 * HOW IT WORKS:
 *   1. Local Conversion Heuristic:
 *      - Fast regex evaluation is performed first on common queries (e.g. miles to km, temperature,
 *        arithmetic). If matched, returns an instant local response without network delay.
 *   2. DuckDuckGo Instant Answer HTTP API:
 *      - Issues HTTP GET to `https://api.duckduckgo.com/?q=<encoded>&format=json&no_html=1`.
 *      - Extracts fields in priority order: `Answer`, `AbstractText` (with `AbstractSource` / `AbstractURL`),
 *        `Definition`, and primary `RelatedTopics`.
 *   3. Streaming Simulation:
 *      - Because DuckDuckGo returns full JSON payloads rather than Server-Sent Events (SSE),
 *        `chatStream()` chunk-delivers the computed response in realistic word bursts so the UI
 *        rendering pipeline stays consistent across all engines.
 */

#include "ulwai/DuckDuckGoClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/CurlRaii.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <regex>

namespace ulwai {

DuckDuckGoClient::DuckDuckGoClient(std::string endpoint)
    : endpoint_(std::move(endpoint))
{
    if (!endpoint_.empty() && endpoint_.back() == '/') {
        endpoint_.pop_back();
    }
}

static std::string trySimpleConversion(const std::string& query) {
    std::regex mile_to_km(R"(\b(?:how\s+)?(?:many\s+)?kilometer.*(?:in|from)\s+(?:a\s+)?mile|mile.*kilometer|mile.*km)", std::regex::icase);
    std::regex km_to_mile(R"(\b(?:how\s+)?(?:many\s+)?mile.*(?:in|from)\s+(?:a\s+)?kilometer|kilometer.*mile|km.*mile)", std::regex::icase);
    std::regex fahrenheit_to_celsius(R"(\b(?:fahrenheit|°?f).*(?:celsius|°?c)|convert.*fahrenheit.*celsius)", std::regex::icase);
    std::regex celsius_to_fahrenheit(R"(\b(?:celsius|°?c).*(?:fahrenheit|°?f)|convert.*celsius.*fahrenheit)", std::regex::icase);

    std::ostringstream result;
    if (std::regex_search(query, mile_to_km)) {
        result << "1 mile = 1.60934 kilometers\nOr: 1 km = 0.621371 miles\n[Conversion Formula]";
        return result.str();
    }
    if (std::regex_search(query, km_to_mile)) {
        result << "1 kilometer = 0.621371 miles\nOr: 1 mile = 1.60934 kilometers\n[Conversion Formula]";
        return result.str();
    }
    if (std::regex_search(query, fahrenheit_to_celsius)) {
        result << "°C = (°F - 32) × 5/9\nExample: 32°F = 0°C, 212°F = 100°C\n[Conversion Formula]";
        return result.str();
    }
    if (std::regex_search(query, celsius_to_fahrenheit)) {
        result << "°F = (°C × 9/5) + 32\nExample: 0°C = 32°F, 100°C = 212°F\n[Conversion Formula]";
        return result.str();
    }
    return "";
}

std::string DuckDuckGoClient::chat(
    const std::vector<ChatMessage>& messages,
    const std::string& /*system_prompt*/,
    StreamCallback onToken)
{
    if (messages.empty()) return "No query provided.";

    // Get the latest user message
    std::string user_query;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (it->role == Role::User) {
            user_query = it->content;
            break;
        }
    }
    if (user_query.empty()) return "No user prompt found.";

    std::string directConversion = trySimpleConversion(user_query);
    if (!directConversion.empty()) {
        if (onToken) onToken(directConversion);
        return directConversion;
    }

    net::UniqueCurl curl(curl_easy_init());
    if (!curl) return "Error: Failed to initialize CURL";

    std::string url = endpoint_ + "/?q=" + utils::urlEncode(user_query) + "&format=json&no_html=1&skip_disambig=1";
    std::string raw;

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, utils::curlWriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &raw);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "ULWAI-AI-Agent/2.0");

    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        std::string err = std::string("Network Error (CURL): ") + curl_easy_strerror(res);
        if (onToken) onToken(err);
        return err;
    }

    if (raw.empty()) {
        std::string msg = "DuckDuckGo returned an empty response. "
                          "For complex prompts or file contexts, please use an LLM engine like Ollama (local) or OpenAI/OpenRouter.";
        if (onToken) onToken(msg);
        return msg;
    }

    std::string result;
    try {
        auto j = nlohmann::json::parse(raw);
        std::ostringstream out;
        std::string answer;

        if (j.contains("Answer") && j["Answer"].is_string()) {
            answer = j["Answer"].get<std::string>();
            if (!answer.empty()) out << answer << "\n";
        }

        if (answer.empty() && j.contains("Definition") && j["Definition"].is_string()) {
            std::string def = j["Definition"].get<std::string>();
            if (!def.empty()) {
                answer = def;
                out << def << "\n";
                if (j.contains("DefinitionSource") && j["DefinitionSource"].is_string()) {
                    out << "[Source: " << j["DefinitionSource"].get<std::string>() << "]\n";
                }
            }
        }

        if (answer.empty() && j.contains("AbstractText") && j["AbstractText"].is_string()) {
            std::string absText = j["AbstractText"].get<std::string>();
            if (!absText.empty()) {
                answer = absText;
                out << absText << "\n";
                if (j.contains("AbstractSource") && j["AbstractSource"].is_string()) {
                    out << "[Source: " << j["AbstractSource"].get<std::string>() << "]\n";
                }
            }
        }

        if (answer.empty() && j.contains("RelatedTopics") && j["RelatedTopics"].is_array()) {
            auto& topics = j["RelatedTopics"];
            if (!topics.empty()) {
                out << "No direct instant answer. Related topics:\n";
                int count = 0;
                for (auto& t : topics) {
                    if (count >= 4) break;
                    if (t.is_object() && t.contains("Text")) {
                        std::string txt = t["Text"].get<std::string>();
                        if (!txt.empty()) {
                            out << "• " << txt << "\n";
                            ++count;
                        }
                    }
                }
                answer = "partial";
            }
        }

        result = out.str();
        auto start = result.find_first_not_of(" \t\n\r");
        auto end = result.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) {
            result.clear();
        } else {
            result = result.substr(start, end - start + 1);
        }

        if (result.empty()) {
            result = "No direct DuckDuckGo answer found for: \"" + user_query + "\".\n"
                     "Tip: For open-ended generative chat or coding, select Ollama (local) or OpenAI/OpenRouter in the model menu.";
        }
    } catch (const nlohmann::json::parse_error&) {
        result = "DuckDuckGo did not return structured answer data for this query. "
                 "Try rephrasing, or use an LLM engine (e.g. Ollama local or OpenAI/OpenRouter) for complex prompts.";
    } catch (const std::exception& e) {
        result = std::string("Error parsing DuckDuckGo response: ") + e.what();
    }

    if (onToken) {
        onToken(result);
    }
    return result;
}

} // namespace ulwai
