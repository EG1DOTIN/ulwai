/**
 * @file CliRunner.cpp
 * @brief Terminal Command-Line Interface, REPL, and Shell Agent Execution.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Manages all terminal interactions for `ulwai`, `ulw-ai-agent`, and `ulwai-cli`.
 *   Provides sub-second, zero-Qt command line execution, including one-shot queries,
 *   code extraction, safe interactive shell command execution, file context ingestion,
 *   and full interactive multi-turn REPL sessions.
 *
 * KEY MODES & CAPABILITIES:
 *   1. One-Shot Query Execution:
 *      - Direct prompt passed as argument: streams tokens live to `stdout` with ANSI styling.
 *   2. Code-Only Mode (`-c`, `--code`):
 *      - Pre-conditions the system prompt and filters output to emit raw, executable code
 *        without chatty narrative or markdown boilerplate.
 *   3. Shell Agent Mode (`-s`, `--shell`):
 *      - Instructs the AI model to generate a precise shell command for the user's intent.
 *      - Parses and displays the proposed command in bright ANSI cyan.
 *      - Pauses for explicit user confirmation (`[y/N]`).
 *      - If approved, executes safely via `platform::executeCommand`, capturing stdout/stderr
 *        and displaying the output.
 *   4. File Attachment Context (`-f <path>`):
 *      - Loads local file contents and injects them alongside the user's question into the context.
 *   5. Interactive REPL (`--cli`):
 *      - Persistent terminal conversation session supporting slash commands:
 *        `/new`, `/engine <name>`, `/model <name>`, `/clear`, `/exit`.
 */

#include "ulwai/cli/CliRunner.hpp"
#include "ulwai/Platform.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/Version.hpp"
#include <iostream>
#include <filesystem>

namespace ulwai::cli {

CliRunner::CliRunner(Config& config, ConversationManager& conversation)
    : config_(config), conversation_(conversation) {
    platform::initConsole();
}

void CliRunner::printVersion() const {
    std::cout << config_.app_name << " v" << ulwai::getVersion() << " (C++20 Ultra-Lightweight AI Agent)\n";
}

void CliRunner::printHelp() const {
    std::cout << "\033[1;36mulwai / " << config_.app_name << " v" << ulwai::getVersion() << "\033[0m – Ultra-Lightweight AI Agent (CLI & Desktop UI)\n\n"
              << "\033[1mUSAGE:\033[0m\n"
              << "  ulwai [OPTIONS] [\"prompt\"]          (or ulw-ai-agent / ulwai-cli)\n"
              << "  ulwai --gui                         Launch Desktop Chat Interface\n"
              << "  ulwai --cli                         Launch Interactive Terminal Chat REPL\n\n"
              << "\033[1mOPTIONS:\033[0m\n"
              << "  -m <engine>           AI engine (pollinations, duckduckgo, ollama, openai, openrouter, grok, huggingface)\n"
              << "  --model <name>        Model name (for pollinations: openai, mistral, llama, qwen, searchgpt)\n"
              << "  -s, --shell           Shell agent mode: generates command and prompts to execute\n"
              << "  -c, --code            Code mode: output code only, minimal explanations\n"
              << "  -f <file>             Attach file content as context with your query\n"
              << "  --system <prompt>     Custom system prompt / instructions\n"
              << "  --api-key <key>       API key (overrides saved config)\n"
              << "  --config [path]       Open config file in default editor (or use specified path)\n"
              << "  -h, --help            Show this help manual\n"
              << "  -v, --version         Show application version\n\n"
              << "\033[1mSUPPORTED ENGINES:\033[0m\n"
              << "  pollinations          Free generative conversational AI (No API key required, Default)\n"
              << "  duckduckgo            Instant search answers & definitions (Free, No API key)\n"
              << "  ollama                Local Ollama / llama.cpp models on localhost:11434 (Free, Offline)\n"
              << "  openai                OpenAI API or any OpenAI-compatible server (vLLM, LM Studio, etc.)\n"
              << "  openrouter            OpenRouter multi-model aggregator (Claude, Llama, Gemini, etc.)\n"
              << "  grok                  xAI Grok API\n"
              << "  huggingface           Hugging Face Inference API\n\n"
              << "\033[1mEXAMPLES:\033[0m\n"
              << "  " << config_.app_name << " \"Explain quantum computing in simple terms\"\n"
              << "  " << config_.app_name << " -m pollinations --model mistral \"Write a sorting algorithm in C++\"\n"
              << "  " << config_.app_name << " -c \"Write a modern C++20 thread-safe queue\"\n"
              << "  " << config_.app_name << " -s \"find all files larger than 100MB in current dir\"\n"
              << "  " << config_.app_name << " -f CMakeLists.txt \"Explain what this CMake configuration does\"\n"
              << "  " << config_.app_name << " -m ollama --model llama3 \"Hello from local AI\"\n"
              << "  " << config_.app_name << " --cli\n";
}

int CliRunner::run(int argc, char* argv[]) {
    std::string engine = config_.default_engine;
    std::string model;
    std::string apiKey;
    std::string systemPrompt = config_.system_prompt;
    std::string filePath;
    bool isShellMode = false;
    bool isCodeMode = false;
    bool openConfig = false;
    std::string customConfigPath;
    std::string userPrompt;
    bool launchInteractive = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "--cli") {
            launchInteractive = true;
        } else if (arg == "-m" && i + 1 < argc) {
            engine = argv[++i];
        } else if (arg == "--model" && i + 1 < argc) {
            model = argv[++i];
        } else if (arg == "--api-key" && i + 1 < argc) {
            apiKey = argv[++i];
        } else if (arg == "-s" || arg == "--shell") {
            isShellMode = true;
        } else if (arg == "-c" || arg == "--code") {
            isCodeMode = true;
        } else if (arg == "-f" && i + 1 < argc) {
            filePath = argv[++i];
        } else if (arg == "--system" && i + 1 < argc) {
            systemPrompt = argv[++i];
        } else if (arg == "--config") {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                customConfigPath = argv[++i];
                (void)config_.load(customConfigPath);
            } else {
                openConfig = true;
            }
        } else if (arg[0] != '-') {
            if (!userPrompt.empty()) userPrompt += " ";
            userPrompt += arg;
        }
    }

    if (openConfig) {
        auto cfgPath = config_.getCurrentLoadedPath();
        if (cfgPath.empty()) cfgPath = platform::getDefaultConfigFilePath();
        std::cout << "Opening config: " << cfgPath << "\n";
        platform::openFileInDefaultEditor(cfgPath);
        return 0;
    }

    if (!apiKey.empty()) {
        if (engine == "openai") config_.openai.api_key = apiKey;
        else if (engine == "openrouter") config_.openrouter.api_key = apiKey;
        else if (engine == "grok") config_.grok.api_key = apiKey;
        else if (engine == "huggingface") config_.huggingface.api_key = apiKey;
    }
    if (!model.empty()) {
        if (engine == "pollinations") config_.pollinations.model = model;
        else if (engine == "openai") config_.openai.model = model;
        else if (engine == "ollama") config_.ollama.model = model;
        else if (engine == "openrouter") config_.openrouter.model = model;
        else if (engine == "grok") config_.grok.model = model;
        else if (engine == "huggingface") config_.huggingface.model = model;
    }

    if (launchInteractive) {
        return runInteractive();
    }

    if (!filePath.empty()) {
        std::string content = utils::readFileContents(filePath);
        if (content.empty()) {
            std::cerr << "Error: Could not read file '" << filePath << "'\n";
            return 1;
        }
        std::string fname = std::filesystem::path(filePath).filename().string();
        userPrompt = "[File Attached: " + fname + "]\n" + content + "\n\nQuery: " + userPrompt;
    }

    if (userPrompt.empty()) {
        printHelp();
        return 0;
    }

    return executeSinglePrompt(userPrompt, engine, systemPrompt, isShellMode, isCodeMode);
}

int CliRunner::executeSinglePrompt(
    const std::string& prompt,
    const std::string& engine,
    const std::string& systemPrompt,
    bool isShellMode,
    bool isCodeMode)
{
    std::string sys = systemPrompt;
    if (isShellMode) {
        sys = "You are a shell command expert. Respond with ONLY the exact single-line raw shell command "
              "that accomplishes the user's request. No explanations, no markdown fences, no formatting.";
    } else if (isCodeMode && sys.empty()) {
        sys = "You are an expert programmer. Provide ONLY the code without explanations or prose.";
    }

    auto client = ConversationManager::createClient(engine, config_);
    if (!client) {
        std::cerr << "Error: Unable to instantiate client for engine '" << engine << "'\n";
        return 1;
    }

    std::cout << "\033[1;32m[" << client->name() << "]\033[0m Processing...\r" << std::flush;

    std::vector<ChatMessage> msgs;
    msgs.push_back({Role::User, prompt, "", {}, isShellMode, ""});

    std::string result;
    if (isShellMode) {
        // Collect full response without streaming to parse command safely
        result = client->chat(msgs, sys, nullptr);
        std::cout << "\r\033[K"; // Clear line

        std::string cmd = utils::stripShellMarkdown(result);
        std::cout << "\n\033[1;33mSuggested Command:\033[0m\n  \033[1;37m" << cmd << "\033[0m\n\n";
        std::cout << "Execute this command? [y/N]: " << std::flush;

        std::string ans;
        std::getline(std::cin, ans);
        if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
            std::cout << "\n\033[1;36m[Executing...]\033[0m\n";
            std::string output;
            int ret = platform::executeCommand(cmd, output);
            std::cout << output;
            if (ret != 0) {
                std::cout << "\n\033[1;31m[Exit code: " << ret << "]\033[0m\n";
            }
            conversation_.addMessage(Role::User, prompt, true);
            conversation_.addMessage(Role::Assistant, result, true, output);
        } else {
            std::cout << "Execution cancelled.\n";
            conversation_.addMessage(Role::User, prompt, true);
            conversation_.addMessage(Role::Assistant, result, true, "Cancelled by user");
        }
    } else {
        std::cout << "\r\033[K";
        // Stream token by token with live output
        auto onToken = [](std::string_view token) {
            std::cout << token << std::flush;
        };
        result = client->chat(msgs, sys, onToken);
        std::cout << "\n";

        conversation_.addMessage(Role::User, prompt);
        conversation_.addMessage(Role::Assistant, result);
    }

    return 0;
}

int CliRunner::runInteractive() {
    std::cout << "\033[1;36m=== " << config_.app_name << " Interactive Terminal Session ===\033[0m\n"
              << "Type your prompt and press Enter.\n"
              << "Commands: /new (new chat), /engine <name>, /model <name>, /clear, /exit\n\n";

    conversation_.newSession("Terminal Session");
    std::string currentEngine = config_.default_engine;

    while (true) {
        std::string currentModel = config_.resolveModel(currentEngine);
        std::cout << "\033[1;32mULWAI [" << currentEngine << ":" << currentModel << "] > \033[0m" << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) break;

        auto trimStart = line.find_first_not_of(" \t");
        if (trimStart == std::string::npos) continue;
        line = line.substr(trimStart);

        if (line == "/exit" || line == "/quit") {
            std::cout << "Goodbye!\n";
            break;
        }
        if (line == "/new" || line == "/reset") {
            conversation_.newSession("Terminal Session");
            std::cout << "\033[1;33mNew conversation session started.\033[0m\n\n";
            continue;
        }
        if (line == "/clear") {
            conversation_.clearCurrentMessages();
            std::cout << "\033[1;33mChat history cleared.\033[0m\n\n";
            continue;
        }
        if (line.starts_with("/engine ")) {
            currentEngine = line.substr(8);
            std::cout << "Engine switched to: " << currentEngine << "\n\n";
            continue;
        }
        if (line.starts_with("/model ")) {
            std::string newModel = line.substr(7);
            if (currentEngine == "openai") config_.openai.model = newModel;
            else if (currentEngine == "ollama") config_.ollama.model = newModel;
            else if (currentEngine == "openrouter") config_.openrouter.model = newModel;
            std::cout << "Model set to: " << newModel << "\n\n";
            continue;
        }

        conversation_.addMessage(Role::User, line);
        auto client = ConversationManager::createClient(currentEngine, config_);
        if (!client) {
            std::cout << "\033[1;31mError creating client for " << currentEngine << "\033[0m\n\n";
            continue;
        }

        std::cout << "\033[1;35mAI: \033[0m";
        auto onToken = [](std::string_view token) {
            std::cout << token << std::flush;
        };

        auto msgs = conversation_.getContextMessages();
        std::string response = client->chat(msgs, config_.system_prompt, onToken);
        std::cout << "\n\n";
        conversation_.addMessage(Role::Assistant, response);
    }

    return 0;
}

} // namespace ulwai::cli
