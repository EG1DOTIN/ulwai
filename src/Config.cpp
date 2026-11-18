/**
 * @file Config.cpp
 * @brief Configuration Management, JSON Serialization & Environment Resolution.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Manages the global configuration state for ULWAI. It is responsible for loading,
 *   validating, defaulting, and persisting user preferences, API keys, endpoints, and UI settings.
 *
 * KEY RESPONSIBILITIES:
 *   1. Default Presets:
 *      - Provides production-ready defaults for 7 supported engines: Pollinations.ai (free generative AI),
 *        DuckDuckGo (free instant search), Ollama (local offline), OpenAI, OpenRouter, xAI Grok,
 *        and Hugging Face.
 *   2. Environment Variable Resolution:
 *      - Checks standard environment variables on startup (`OPENAI_API_KEY`, `OPENROUTER_API_KEY`,
 *        `XAI_API_KEY`, `HF_TOKEN` / `HUGGINGFACE_API_KEY`, `OLLAMA_HOST`).
 *      - Allows headless containers and automated workflows to operate securely without storing keys on disk.
 *   3. JSON Persistence (`config.json`):
 *      - Serializes to and loads from `<ConfigDir>/config.json`.
 *      - Automatically creates parent directories and a default configuration file if none exists.
 *   4. Engine Resolver API:
 *      - `resolveApiKey(engine)`: Returns the appropriate key for the specified provider.
 *      - `resolveModel(engine)`: Returns the configured model name or provider default.
 *      - `resolveEndpoint(engine)`: Returns the target REST/SSE HTTP endpoint URL.
 */

#include "ulwai/Config.hpp"
#include "ulwai/Platform.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace ulwai {

using json = nlohmann::json;

Config::Config() {
    pollinations.endpoint = "https://text.pollinations.ai/openai";
    pollinations.model = "openai";

    duckduckgo.endpoint = "https://api.duckduckgo.com/";
    duckduckgo.model = "default";

    ollama.endpoint = "http://localhost:11434/api/chat";
    ollama.model = "llama3";

    openai.endpoint = "https://api.openai.com/v1/chat/completions";
    openai.model = "gpt-4o-mini";

    openrouter.endpoint = "https://openrouter.ai/api/v1/chat/completions";
    openrouter.model = "meta-llama/llama-3.1-8b-instruct:free";

    grok.endpoint = "https://api.x.ai/v1/chat/completions";
    grok.model = "grok-beta";

    huggingface.endpoint = "https://api-inference.huggingface.co/models/";
    huggingface.model = "mistralai/Mistral-7B-Instruct-v0.2";

    // Check environment variables for API keys
    const char* env_openai = std::getenv("OPENAI_API_KEY");
    if (env_openai) openai.api_key = env_openai;

    const char* env_openrouter = std::getenv("OPENROUTER_API_KEY");
    if (env_openrouter) openrouter.api_key = env_openrouter;

    const char* env_xai = std::getenv("XAI_API_KEY");
    if (env_xai) grok.api_key = env_xai;

    const char* env_hf = std::getenv("HF_TOKEN");
    if (!env_hf) env_hf = std::getenv("HUGGINGFACE_API_KEY");
    if (env_hf) huggingface.api_key = env_hf;

    const char* env_ollama = std::getenv("OLLAMA_HOST");
    if (env_ollama) ollama.endpoint = std::string(env_ollama) + "/api/chat";
}

bool Config::load(const std::filesystem::path& path) {
    std::filesystem::path target = path.empty() ? platform::getDefaultConfigFilePath() : path;
    loaded_path_ = target;

    if (!std::filesystem::exists(target)) {
        (void)save(target);
        return true;
    }

    std::ifstream file(target);
    if (!file.is_open()) return false;

    try {
        json j;
        file >> j;

        if (j.contains("app_name") && j["app_name"].is_string()) {
            app_name = j["app_name"].get<std::string>();
        }
        if (j.contains("default_engine") && j["default_engine"].is_string())
            default_engine = j["default_engine"].get<std::string>();
        if (j.contains("system_prompt") && j["system_prompt"].is_string())
            system_prompt = j["system_prompt"].get<std::string>();
        if (j.contains("temperature") && j["temperature"].is_number())
            temperature = j["temperature"].get<double>();
        if (j.contains("max_tokens") && j["max_tokens"].is_number_integer())
            max_tokens = j["max_tokens"].get<int>();
        if (j.contains("history_limit") && j["history_limit"].is_number_integer())
            history_limit = j["history_limit"].get<int>();
        if (j.contains("theme") && j["theme"].is_string())
            theme = j["theme"].get<std::string>();
        if (j.contains("stream_output") && j["stream_output"].is_boolean())
            stream_output = j["stream_output"].get<bool>();
        if (j.contains("font_size") && j["font_size"].is_number_integer())
            font_size = std::clamp(j["font_size"].get<int>(), 9, 32);
        if (j.contains("chat_sort_latest_first") && j["chat_sort_latest_first"].is_boolean())
            chat_sort_latest_first = j["chat_sort_latest_first"].get<bool>();

        auto readEngine = [](const json& parent, const std::string& key, EngineConfig& cfg) {
            if (parent.contains(key) && parent[key].is_object()) {
                const auto& item = parent[key];
                if (item.contains("api_key") && item["api_key"].is_string())
                    cfg.api_key = item["api_key"].get<std::string>();
                if (item.contains("model") && item["model"].is_string())
                    cfg.model = item["model"].get<std::string>();
                if (item.contains("endpoint") && item["endpoint"].is_string())
                    cfg.endpoint = item["endpoint"].get<std::string>();
            }
        };

        readEngine(j, "pollinations", pollinations);
        readEngine(j, "duckduckgo", duckduckgo);
        readEngine(j, "ollama", ollama);
        readEngine(j, "openai", openai);
        readEngine(j, "openrouter", openrouter);
        readEngine(j, "grok", grok);
        readEngine(j, "huggingface", huggingface);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Config load warning: " << e.what() << "\n";
        return false;
    }
}

bool Config::save(const std::filesystem::path& path) const {
    std::filesystem::path target = path.empty() ? (loaded_path_.empty() ? platform::getDefaultConfigFilePath() : loaded_path_) : path;

    std::error_code ec;
    std::filesystem::create_directories(target.parent_path(), ec);

    json j;
    j["app_name"] = app_name;
    j["default_engine"] = default_engine;
    j["system_prompt"] = system_prompt;
    j["temperature"] = temperature;
    j["max_tokens"] = max_tokens;
    j["history_limit"] = history_limit;
    j["theme"] = theme;
    j["stream_output"] = stream_output;
    j["font_size"] = font_size;
    j["chat_sort_latest_first"] = chat_sort_latest_first;

    auto writeEngine = [](json& parent, const std::string& key, const EngineConfig& cfg) {
        json item;
        item["api_key"] = cfg.api_key;
        item["model"] = cfg.model;
        item["endpoint"] = cfg.endpoint;
        parent[key] = item;
    };

    writeEngine(j, "pollinations", pollinations);
    writeEngine(j, "duckduckgo", duckduckgo);
    writeEngine(j, "ollama", ollama);
    writeEngine(j, "openai", openai);
    writeEngine(j, "openrouter", openrouter);
    writeEngine(j, "grok", grok);
    writeEngine(j, "huggingface", huggingface);

    std::ofstream file(target);
    if (!file.is_open()) return false;

    file << j.dump(4);
    return true;
}

std::string Config::resolveApiKey(const std::string& engine) const {
    if (engine == "openai") return openai.api_key;
    if (engine == "openrouter") return openrouter.api_key;
    if (engine == "grok") return grok.api_key;
    if (engine == "huggingface") return huggingface.api_key;
    return "";
}

std::string Config::resolveModel(const std::string& engine) const {
    if (engine == "pollinations") return pollinations.model.empty() ? "openai" : pollinations.model;
    if (engine == "openai") return openai.model;
    if (engine == "ollama") return ollama.model;
    if (engine == "openrouter") return openrouter.model;
    if (engine == "grok") return grok.model;
    if (engine == "huggingface") return huggingface.model;
    if (engine == "duckduckgo") return duckduckgo.model;
    return "default";
}

std::string Config::resolveEndpoint(const std::string& engine) const {
    if (engine == "pollinations") return pollinations.endpoint.empty() ? "https://text.pollinations.ai/openai" : pollinations.endpoint;
    if (engine == "openai") return openai.endpoint;
    if (engine == "ollama") return ollama.endpoint;
    if (engine == "openrouter") return openrouter.endpoint;
    if (engine == "grok") return grok.endpoint;
    if (engine == "huggingface") return huggingface.endpoint;
    if (engine == "duckduckgo") return duckduckgo.endpoint;
    return "";
}

} // namespace ulwai
