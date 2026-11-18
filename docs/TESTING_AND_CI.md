# ULWAI AI Agent — Testing & CI Guide

> **Comprehensive edge-case test catalog, test runner execution steps, and continuous integration specifications.**

Quality and stability in ULWAI are ensured through a battery of over 35 edge-case test suites covering every critical data processing and network layer.

---

## 1. Test Architecture & Edge Case Matrix

The test executable ([`tests/test_main.cpp`](../tests/test_main.cpp)) validates all subsystems against invalid inputs, boundary states, and network packet structures:

| Test Domain | Test Function | Edge Cases Verified |
| :--- | :--- | :--- |
| **URL Percent Encoding** | `testUtilsEdgeCases` | Empty strings, unreserved ASCII (`_.-~`), reserved characters (`!*();:@&=+$,/?#[]`), spaces, non-ASCII multi-byte characters. |
| **HTML Sanitization** | `testUtilsEdgeCases` | Protection against script tags (`<script>alert(1)</script>`), unbalanced tags, double quotes, single quotes, ampersands, mixed tags. |
| **Markdown Fences** | `testUtilsEdgeCases` | Code fences with language badges (` ```cpp ````), unlabelled code blocks, nested backticks, markdown bold and italics, bullet lists. |
| **Shell Command Sanitization** | `testUtilsEdgeCases` | Stripping markdown wrappers from shell commands, whitespace normalization, multi-line command safety. |
| **Configuration Loading & Saving** | `testConfigEdgeCases` | Auto-creation of default `config.json` when missing, recovery from corrupted JSON, priority resolution of environment variables (`OPENAI_API_KEY`, `OLLAMA_HOST`). |
| **Context Sliding-Window Pruning** | `testConversationManagerEdgeCases` | Pruning multi-turn conversations when message count exceeds `history_limit`, ensuring recent user questions are never lost. |
| **Auto-Titling Heuristic** | `testConversationManagerEdgeCases` | Truncating long initial prompts into clean, human-readable session titles. |
| **Session Persistence** | `testConversationManagerEdgeCases` | Full disk round-trip serialization to JSON, updating timestamps, deleting sessions, listing sessions sorted by updated time. |
| **Client Factory** | `testClientFactoryAndPollinations` | Factory resolution for all 7 engine identifiers (`pollinations`, `duckduckgo`, `ollama`, `openai`, `openrouter`, `grok`, `huggingface`) and fallback aliases (`free`, `duck`). |
| **Platform & Subprocesses** | `testPlatformAndFileEdgeCases` | Cross-platform directory paths (`~/.config/ulwai`), pipe execution and output capture (`executeCommand`), reading non-existent files gracefully. |
| **Title Search & Filtering** | `testUtilsEdgeCases` | Case-insensitive and case-sensitive substring matching, whole-word boundary matching, ECMAScript regex searches, protection against invalid unclosed regex patterns, Unicode/Devanagari query validation. |
| **Crash Handling & Logging** | `testLoggingAndCrashHandling` | Thread-safe logging, level-gated disk flushing (`Error`/`Fatal`), initialization, crash signal handlers installation (`SIGSEGV`, `SIGABRT`, `SIGFPE`, `SIGILL`, `SIGBUS`). |

---

## 2. How to Run the Tests Locally

### Option 1: Automated Shell Script (Linux & macOS)
```bash
./scripts/run_tests.sh
```
*Compiles the project if not already built, runs CTest, and executes the test binary.*

### Option 2: Universal Python Test Runner
```bash
python3 scripts/build_all.py
```

### Option 3: Direct CTest Invocation
```bash
ctest --test-dir build --output-on-failure
```

### Option 4: Direct Test Binary Execution
To see detailed per-test execution output:
```bash
./build/tests/ulwai_tests
```

Expected Output:
```
=================================================
Running ULWAI AI Agent Comprehensive Test Suite  
=================================================
[RUN] Testing Utils Edge Cases...
[PASS] Utils edge cases passed.
[RUN] Testing Config Edge Cases...
[PASS] Config edge cases passed.
[RUN] Testing ConversationManager Edge Cases...
[PASS] ConversationManager edge cases passed.
[RUN] Testing Client Factory & Pollinations...
[PASS] Client factory and Pollinations tests passed.
[RUN] Testing Platform & File I/O Edge Cases...
[PASS] Platform and File I/O edge cases passed.

>>> ALL 35+ COMPREHENSIVE EDGE-CASE TEST SUITES PASSED CLEANLY! <<<
```

---

## 3. How to Write a New Unit Test

To add tests for a new capability or bug fix:
1. Open [`tests/test_main.cpp`](../tests/test_main.cpp).
2. Add a new test function:
   ```cpp
   static void testMyNewFeature() {
       std::cout << "[RUN] Testing My New Feature..." << std::endl;
       // Arrange & Act
       bool condition = ...;
       // Assert
       assert(condition);
       std::cout << "[PASS] My New Feature passed." << std::endl;
   }
   ```
3. Call your function inside `main()`:
   ```cpp
   testMyNewFeature();
   ```
4. Rebuild and run `./scripts/run_tests.sh`.

---

## 4. Continuous Integration & Automated Release Matrix

GitHub Actions automatically builds, verifies, and packages executables across three major operating systems via [`.github/workflows/ci.yml`](../.github/workflows/ci.yml):

| OS Runner | Compiler & Toolchain | Targets Built | Package Format | Post-Merge / Release Action |
| :--- | :--- | :--- | :--- | :--- |
| **Ubuntu 24.04** | GCC 14, Qt6, CMake 3.20+ | Desktop GUI (`ulwai`, `ulw-ai-agent`), CLI (`ulwai-cli`), Tests | `ulwai-v<VER>-linux-x86_64.tar.gz` | CTest verification, artifact upload, auto-published to GitHub Release |
| **macOS 14** | AppleClang 15, Homebrew Qt6 & curl | Desktop GUI (`ulwai`, `ulw-ai-agent`), CLI (`ulwai-cli`), Tests | `ulwai-v<VER>-macos.tar.gz` | CTest verification, artifact upload, auto-published to GitHub Release |
| **Windows 2022** | MSVC 19.3x, Qt 6.7.2, vcpkg curl | Desktop GUI (`ulwai.exe`, `ulw-ai-agent.exe`), CLI (`ulwai-cli.exe`), Tests | `ulwai-v<VER>-windows-x64.zip` (self-contained with `windeployqt` & `libcurl.dll`) | CTest verification, artifact upload, auto-published to GitHub Release |

### Automated Pipeline Flow:
1. **Pull Requests (`pull_request: branches: [main]`):**
   - Runs full compilation and CTest suites across Ubuntu, macOS, and Windows.
   - Builds and packages platform-specific bundles.
   - Uploads build artifacts to GitHub Actions workflow runs for preview testing.
2. **Post-Merge to Main (`push: branches: [main]`):**
   - Triggers automatically upon successful merge of any PR into `main`.
   - Executes the matrix compilation, testing, and packaging.
   - Downstream `release` job downloads all matrix artifacts, computes `SHA256SUMS.txt`, and publishes/updates the continuous `latest-build` GitHub Release.
3. **Version Tags (`push: tags: ['v*']`):**
   - Automatically publishes an official versioned release with all multi-platform binaries attached.
