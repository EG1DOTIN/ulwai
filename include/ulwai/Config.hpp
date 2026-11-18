/**
 * @file Config.hpp
 * @brief Application Settings, Engine Configuration Structures & Resolution API.
 *
 * Encapsulates all persistent user preferences, AI engine credentials,
 * generation parameters, and file persistence interfaces for ULWAI.
 */

#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace ulwai {

struct EngineConfig {
    std::string api_key;
    std::string model;
    std::string endpoint;
    std::string extra_header;
};

class Config {
public:
    Config();

    std::string app_name{"ulw-ai-agent"};
    std::string default_engine{"pollinations"};
    std::string system_prompt;
    double temperature{0.7};
    int max_tokens{2048};
    int history_limit{20};
    std::string theme{"dark"};
    bool stream_output{true};
    int font_size{13};
    bool chat_sort_latest_first{false}; // Default: Chronological (User prompt first, then AI response, like ChatGPT/Gemini)

    EngineConfig pollinations;
    EngineConfig duckduckgo;
    EngineConfig ollama;
    EngineConfig openai;
    EngineConfig openrouter;
    EngineConfig grok;
    EngineConfig huggingface;

    [[nodiscard]] bool load(const std::filesystem::path& path = "");
    [[nodiscard]] bool save(const std::filesystem::path& path = "") const;

    [[nodiscard]] std::string resolveApiKey(const std::string& engine) const;
    [[nodiscard]] std::string resolveModel(const std::string& engine) const;
    [[nodiscard]] std::string resolveEndpoint(const std::string& engine) const;

    [[nodiscard]] std::filesystem::path getCurrentLoadedPath() const noexcept {
        return loaded_path_;
    }

private:
    std::filesystem::path loaded_path_;
};

} // namespace ulwai
