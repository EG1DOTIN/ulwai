/**
 * @file main_cli.cpp
 * @brief Standalone Pure-Terminal CLI Binary Entry Point (`ulwai-cli`).
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   This is the dedicated entry point for `ulwai-cli`. Unlike `main.cpp`, this file has
 *   ZERO dependencies on Qt, GUI frameworks, or display servers. It is specifically designed
 *   for deployment in:
 *     - Headless Linux servers, cloud VMs, and Docker/OCI containers.
 *     - Minimalist Linux distributions (Alpine, Debian Minimal, Arch, Void).
 *     - CI/CD build runners and automation pipelines.
 *     - Remote SSH sessions without X11/Wayland forwarding.
 *
 * EXECUTION WORKFLOW:
 *   1. Instantiate `ulwai::Config` and load configuration from disk / environment.
 *   2. Instantiate `ulwai::ConversationManager`.
 *   3. Dispatch directly to `ulwai::cli::CliRunner::run(argc, argv)`.
 *
 * PERFORMANCE FOOTPRINT:
 *   - Binary size: ~700 KB stripped.
 *   - Runtime RAM: ~5 to 15 MB.
 *   - Cold startup latency: < 5 milliseconds.
 */

#include "ulwai/Config.hpp"
#include "ulwai/ConversationManager.hpp"
#include "ulwai/cli/CliRunner.hpp"
#include "ulwai/Logger.hpp"

int main(int argc, char* argv[]) {
    // 0. Initialize logging and crash handlers
    ulwai::initLogging();
    ulwai::installCrashHandler();

    // 1. Load configuration from disk / environment variables
    ulwai::Config config;
    (void)config.load();

    // 2. Initialize chat conversation state manager
    ulwai::ConversationManager conversation(config);

    // 3. Delegate argument parsing, REPL, and prompt execution to CliRunner
    ulwai::cli::CliRunner cliRunner(config, conversation);
    int res = cliRunner.run(argc, argv);
    ulwai::shutdownLogging();
    return res;
}
