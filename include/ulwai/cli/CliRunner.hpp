/**
 * @file CliRunner.hpp
 * @brief Terminal Command-Line Interface & Interactive REPL Engine.
 */

#pragma once

#include "ulwai/Config.hpp"
#include "ulwai/ConversationManager.hpp"
#include <string>
#include <vector>

namespace ulwai::cli {

class CliRunner {
public:
    explicit CliRunner(Config& config, ConversationManager& conversation);

    [[nodiscard]] int run(int argc, char* argv[]);

private:
    Config& config_;
    ConversationManager& conversation_;

    void printHelp() const;
    void printVersion() const;
    [[nodiscard]] int runInteractive();
    [[nodiscard]] int executeSinglePrompt(
        const std::string& prompt,
        const std::string& engine,
        const std::string& systemPrompt,
        bool isShellMode,
        bool isCodeMode);
};

} // namespace ulwai::cli
