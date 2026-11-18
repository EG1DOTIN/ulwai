# ULWAI (ulw-ai-agent) — Ultra-Lightweight AI Agent

[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](VERSION)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-purple.svg)](CMakeLists.txt)
[![RAM Usage](https://img.shields.io/badge/RAM-%3C100MB-success.svg)](docs/ARCHITECTURE.md)

> **Current Version:** `v2.0.0` (Managed via single source of truth in [`VERSION`](VERSION))
> 
> **High-performance, native C++20 AI Agent for Desktop & Terminal — Operates strictly under 100 MB of RAM with full UI.**

**ULWAI** (`ulw-ai-agent`) is a high-performance, native C++20 desktop and terminal AI assistant. It combines an intuitive desktop chat experience with a fast, powerful terminal interface, featuring zero heavy runtimes (no Electron, no webview bloat, no Node.js, no Python overhead) and sub-second startup times.

---

## ⚡ Key Highlights

- **Ultra-Lightweight Footprint:** Peak memory usage with active GUI is **~40–65 MB of RAM** (well below the 100 MB strict budget), compared to 400–800 MB for typical Electron-based AI frontends.
- **Dual Mode (UI & CLI):**
  - **Desktop UI Mode:** Modern ChatGPT / LibreChat-inspired dark interface with multi-turn conversation memory, real-time token streaming, syntax-styled code blocks with one-click copy, collapsible history sidebar, live conversation title search with advanced filters (`[Aa]` Case-Sensitive, `[W]` Whole Word, `[.*]` Regex), and file attachment support.
  - **Terminal / CLI Mode:** One-shot queries, code mode (`-c`), safe shell command generation & execution (`-s`), and interactive REPL shell (`--cli`).
- **High-Performance Architecture:**
  - **$O(1)$ Incremental Message Appending:** Renders new conversation turns via `QTextCursor` incremental DOM insertion without re-parsing all previous messages.
  - **In-Memory Metadata Caching:** Sidebar queries and history navigation resolve in sub-millisecond time without repeated disk scans or full JSON DOM parses.
  - **libcurl RAII Wrappers:** Smart pointer wrappers (`UniqueCurl`, `UniqueCurlSlist`) ensure zero handle leaks on network cancellations or exceptions.
  - **Direct Stream HTML Escaping:** Direct buffer streaming into MD4C Markdown renderers without intermediate heap allocations.
- **Comprehensive Model & Engine Support (7 Backends):**
  - **Pollinations.ai (Default Free Generative AI):** Free, fast cloud AI text & code generation with zero API keys required and multiple model options (`openai`, `openai-fast`, `gpt-oss`).
  - **DuckDuckGo:** Free, instant search definitions, facts, math, unit conversions (no API key required).
  - **Ollama / llama.cpp:** Local offline private inference on `localhost:11434` or custom endpoints.
  - **OpenAI & Compatible:** Full support for OpenAI (`gpt-4o-mini`, `gpt-4o`, `gpt-3.5-turbo`) or any OpenAI-compatible API (LM Studio, vLLM, llama.cpp server, Groq, DeepSeek, LocalAI).
  - **OpenRouter:** Access hundreds of top open-source and proprietary models through a single aggregator.
  - **xAI Grok:** Native xAI Grok API support (`grok-beta`, `grok-2`).
  - **Hugging Face:** Hugging Face Serverless Inference API.
- **Shell Agent Mode:** Generates commands with interactive confirmation dialogs in UI and terminal (`[y/N]`), safely executing approved commands and capturing console output back into the conversation context.
- **Portability:** Strictly cross-platform C++20 (Linux, Windows, macOS). No hardcoded system paths; all configurations and history are managed relative to user config standards (`~/.config/ulwai` on Linux/macOS, `%APPDATA%\ulwai` on Windows).
- **Standalone CLI Binary (`ulwai-cli`):** Zero Qt dependencies; can be compiled and deployed on headless servers, containers, or legacy systems with tiny footprint (<15 MB RAM, ~700 KB binary).

---

## 📚 Technical Documentation Directory

Comprehensive architectural and engineering documentation is available in the [`docs/`](docs/) directory:

| Document | Description |
| :--- | :--- |
| 🏗️ [**Architecture & Design**](docs/ARCHITECTURE.md) | Component hierarchy, dual-mode branching, memory optimizations (<100MB RAM), and Mermaid diagrams. |
| ⚡ [**Query Flow & Streaming**](docs/QUERY_FLOW.md) | Sequence diagrams of async `AiWorker` streaming, SSE chunk parsing, and safe shell execution approval loop. |
| 🔌 [**Engine & Provider Guide**](docs/ENGINE_GUIDE.md) | In-depth guide to all 7 backends (Pollinations, DuckDuckGo, Ollama, OpenAI, OpenRouter, Grok, Hugging Face). |
| 🧪 [**Testing & CI Matrix**](docs/TESTING_AND_CI.md) | Edge-case test catalog (35+ suites) and multi-OS GitHub Actions workflow specification. |
| 📦 [**Third-Party Libraries**](THIRD_PARTY.md) | Inventory of minimal external libraries (MD4C, nlohmann/json, libcurl, Qt 6) and zero-bloat philosophy. |

---

## 🚀 Quick Start in 60 Seconds (Free AI, Zero Setup)

You do **not** need an API key, account, or credit card to start using ULWAI immediately. It comes pre-configured with **Pollinations.ai** and **DuckDuckGo** out of the box:

```bash
# 1. Clone the repository
git clone https://github.com/EG1DOTIN/ulwai.git
cd ulwai

# 2. Build the project
./scripts/build.sh

# 3. Ask a question immediately (CLI)
./build/ulwai "Explain how std::span works in C++20 with an example"

# 4. Or launch the Desktop GUI
./build/ulwai
```

---

## 📦 How to Install Prerequisites

### 1. Ubuntu / Debian / Linux Mint
```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev libcurl4-openssl-dev
```
*(If Qt6 is not packaged on older distributions such as Ubuntu 20.04, install `qtbase5-dev` instead — ULWAI supports both Qt6 and Qt5!)*

### 2. Fedora / Red Hat Enterprise Linux / CentOS
```bash
sudo dnf install -y gcc-c++ cmake qt6-qtbase-devel libcurl-devel
```

### 3. Arch Linux / Manjaro
```bash
sudo pacman -S --needed base-devel cmake qt6-base curl
```

### 4. macOS (via Homebrew)
```bash
brew install cmake qt@6 curl
```

### 5. Windows (MSVC 2019+ or MinGW-w64)
- Install **Visual Studio 2022 Community** with the *"Desktop development with C++"* workload.
- Install CMake (3.20+).
- Install Qt6 (or Qt 5.15) via the official Qt Online Installer or vcpkg (`vcpkg install qtbase curl`).

---

## 🛠️ How to Build the Project

### Option A: Standard Full Build (Desktop GUI + Terminal CLI)
```bash
# Configure Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile using all available CPU cores
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run test suite
ctest --test-dir build --output-on-failure
```

The build produces three binaries in `build/`:
- `ulwai`: Short alias binary (launches GUI or CLI depending on arguments).
- `ulw-ai-agent`: Canonical full application binary name.
- `ulwai-cli`: Dedicated pure-terminal binary (**zero Qt dependencies**).

### Option B: Build Pure-Terminal CLI Only (Zero Qt Dependencies)
For headless servers, Docker containers, or minimal systems without display libraries:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF
cmake --build build -j
```
*This produces only `ulwai-cli` and core libraries, requiring only `libcurl` and standard C++20 libraries.*

### Option C: Universal Python Build & Test Orchestrator
```bash
# Build and automatically run test suite
python3 scripts/build_all.py

# Build CLI-only without GUI
python3 scripts/build_all.py --no-gui

# Clean rebuild
python3 scripts/build_all.py --clean
```

### Option D: Build with Debugging & Sanitizers (ASan + UBSan)
```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
cmake --build build-debug -j
ctest --test-dir build-debug --output-on-failure
```

---

## 🖥️ How to Use the Desktop GUI

Run `./build/ulwai` or `./build/ulw-ai-agent` without arguments in an active desktop session:
```bash
./build/ulwai
# or explicitly:
./build/ulwai --gui
```

### Key Desktop Features & How to Use Them:
1. **Starting a New Chat:**
   - Click `＋ New Chat` at the top of the left sidebar to start a clean session.
2. **Switching AI Models & Engines:**
   - Use the dropdown in the top navigation bar to select between **Pollinations (Free)**, **DuckDuckGo**, **Ollama (Local)**, **OpenAI**, **OpenRouter**, **Grok**, or **Hugging Face**.
3. **One-Click Code Copy:**
   - Every generated code block is displayed with syntax highlighting and a language badge. Hover over the top bar of the code block and click `Copy` to immediately copy the snippet to your system clipboard.
4. **Attaching Files for Analysis:**
   - Click the `📎` paperclip button next to the prompt box to attach source code, config files, or logs. ULWAI injects the file contents into your prompt with clear context delimiters.
5. **Interactive Shell Agent Mode:**
   - Toggle the `Shell Agent` option to request system commands. When the AI proposes a shell command, ULWAI displays a safety confirmation dialog showing the exact command. Clicking **Execute** runs the command locally and feeds the console output back into the conversation context.
6. **Zooming & Text Scaling:**
   - Hold `Ctrl` and scroll your mouse wheel (`Ctrl + Scroll`) inside the chat view to smoothly adjust font size from 9pt to 32pt.
7. **Session Management & Renaming:**
   - Right-click any conversation in the left sidebar to **Rename** or **Delete** it.
8. **Exporting Conversations:**
   - Click `⤓ Export` to export your current conversation as clean GitHub-Flavored Markdown (`.md`), styled HTML (`.html`), or universal JSON (`.json`).

---

## 💻 How to Use Terminal Mode (CLI)

You can run terminal commands using `./build/ulwai` or the standalone `./build/ulwai-cli`.

### 1. One-Shot Instant Answers
Pass your prompt directly as a command-line argument:
```bash
# Factual query via DuckDuckGo / Pollinations (Instant, free)
./build/ulwai "1 mile in km"

# Technical explanation
./build/ulwai "Explain RAII and std::unique_ptr in C++"
```

### 2. Code-Only Mode (`-c`, `--code`)
Forces the AI to generate only executable code with zero conversational chatter or markdown fences:
```bash
./build/ulwai -c "Python script to compute MD5 hash of a file" > hash.py
./build/ulwai -c "quicksort implementation in modern C++20"
```

### 3. Safe Shell Agent Mode (`-s`, `--shell`)
Instructs the AI to translate your natural language goal into a system shell command, displays the command with syntax highlighting, and asks for confirmation:
```bash
./build/ulwai -s "find all files larger than 100MB in /var/log"
```
Output:
```
[AI Proposed Command]:
  find /var/log -type f -size +100M

Execute command? [y/N]: y
[OUTPUT]:
/var/log/syslog.1
/var/log/journal/system.journal
```

### 4. File Attachment Context (`-f <file>`)
Injects local file contents into the prompt context for explanation, debugging, or code review:
```bash
./build/ulwai -f CMakeLists.txt "Summarize all external dependencies required by this build"
./build/ulwai -f src/Platform.cpp "Audit this code for potential memory leaks or dangling pointers"
```

### 5. Interactive Terminal Chat REPL (`--cli`)
Launch an interactive, multi-turn conversational session right in your terminal:
```bash
./build/ulwai --cli
```
Available in-session REPL commands:
- `/new` – Start a new conversation thread
- `/engine <name>` – Switch active engine (e.g. `/engine ollama` or `/engine openai`)
- `/model <name>` – Switch active model (e.g. `/model gpt-4o`)
- `/clear` – Clear conversation context memory
- `/exit` or `Ctrl+C` – Exit interactive session

---

## 🔌 How to Configure Each of the 7 AI Engines

### 1. Pollinations.ai (Free Generative AI — Default)
- **Status:** 100% Free, zero account, zero API keys required.
- **Default Model:** `openai` (GPT-OSS 20B reasoning model).
- **Available Models:** `openai`, `openai-fast`, `gpt-oss`.
- **CLI Example:**
  ```bash
  ./build/ulwai -m pollinations --model openai-fast "Write a binary search function"
  ```

### 2. DuckDuckGo Instant Answer
- **Status:** 100% Free, zero API keys.
- **Best For:** Wikipedia abstracts, unit conversions, math calculations, rapid facts.
- **CLI Example:**
  ```bash
  ./build/ulwai -m duckduckgo "100 Fahrenheit to Celsius"
  ```

### 3. Ollama / llama.cpp (100% Offline Private Local AI)
- **Prerequisites:** Install Ollama (`curl -fsSL https://ollama.com/install.sh | sh`) and pull a model:
  ```bash
  ollama pull llama3
  ```
- **CLI Example:**
  ```bash
  ./build/ulwai -m ollama --model llama3 "Explain memory barriers in C++"
  ```
- **Custom Port or Host:**
  ```bash
  export OLLAMA_HOST="http://localhost:11434"
  ```

### 4. OpenAI & OpenAI-Compatible Endpoints
- **Status:** Requires API key or local server.
- **Compatible Servers:** LM Studio, vLLM, `llama.cpp server`, LocalAI, Groq, DeepSeek.
- **Setup via Environment Variable:**
  ```bash
  export OPENAI_API_KEY="sk-..."
  ./build/ulwai -m openai --model gpt-4o-mini "Explain quantum superposition"
  ```
- **Using Local LM Studio / llama.cpp server (`http://localhost:1234/v1`):**
  Configure the endpoint in `~/.config/ulwai/config.json`:
  ```json
  "openai": {
    "endpoint": "http://localhost:1234/v1/chat/completions",
    "model": "local-model"
  }
  ```

### 5. OpenRouter (Multi-Model Cloud Gateway)
- **Status:** Requires API key (`https://openrouter.ai/keys`). Access Claude 3.5, GPT-4, Llama 3, Gemini.
- **Free Tier Models:** Supports completely free models like `meta-llama/llama-3.1-8b-instruct:free`.
- **Setup:**
  ```bash
  export OPENROUTER_API_KEY="sk-or-..."
  ./build/ulwai -m openrouter --model "meta-llama/llama-3.1-8b-instruct:free" "Hello!"
  ```

### 6. xAI Grok
- **Status:** Requires API key from `https://console.x.ai`.
- **Setup:**
  ```bash
  export XAI_API_KEY="xai-..."
  ./build/ulwai -m grok --model grok-beta "What are the latest discoveries in astrophysics?"
  ```

### 7. Hugging Face Inference API
- **Status:** Requires token from `https://huggingface.co/settings/tokens`.
- **Setup:**
  ```bash
  export HF_TOKEN="hf_..."
  ./build/ulwai -m huggingface --model "mistralai/Mistral-7B-Instruct-v0.2" "Hi Mistral"
  ```

---

## ⚙️ Configuration & Storage Conventions

All user settings, preferences, and session histories are saved strictly according to standard OS user directory conventions:
- **Linux / macOS:** `~/.config/ulwai/`
  - Configuration: `~/.config/ulwai/config.json`
  - Session history: `~/.config/ulwai/history/<session_id>.json`
- **Windows:** `%APPDATA%\ulwai\`
  - Configuration: `%APPDATA%\ulwai\config.json`
  - Session history: `%APPDATA%\ulwai\history\<session_id>.json`

### Opening Configuration in Default Editor:
```bash
./build/ulwai --config
```

### Complete `config.json` Schema Reference:
```json
{
  "app_name": "ulw-ai-agent",
  "default_engine": "pollinations",
  "system_prompt": "You are ULWAI, an expert, concise C++ and software engineering assistant.",
  "temperature": 0.7,
  "max_tokens": 2048,
  "history_limit": 20,
  "theme": "dark",
  "stream_output": true,
  "font_size": 13,
  "pollinations": {
    "endpoint": "https://text.pollinations.ai/openai",
    "model": "openai"
  },
  "ollama": {
    "endpoint": "http://localhost:11434/api/chat",
    "model": "llama3"
  },
  "openai": {
    "api_key": "sk-...",
    "endpoint": "https://api.openai.com/v1/chat/completions",
    "model": "gpt-4o-mini"
  },
  "openrouter": {
    "api_key": "sk-or-...",
    "endpoint": "https://openrouter.ai/api/v1/chat/completions",
    "model": "meta-llama/llama-3.1-8b-instruct:free"
  },
  "grok": {
    "api_key": "xai-...",
    "endpoint": "https://api.x.ai/v1/chat/completions",
    "model": "grok-beta"
  },
  "huggingface": {
    "api_key": "hf_...",
    "endpoint": "https://api-inference.huggingface.co/models/",
    "model": "mistralai/Mistral-7B-Instruct-v0.2"
  }
}
```

---

## 🧪 How to Run Tests & Verify the Build

The project includes an extensive test suite ([`tests/test_main.cpp`](tests/test_main.cpp)) covering 35+ edge cases:

```bash
# Method 1: Using the automated test runner script
./scripts/run_tests.sh

# Method 2: Using CTest
ctest --test-dir build --output-on-failure

# Method 3: Running the test binary directly
./build/tests/ulwai_tests
```

### What is Tested:
- **URL Percent Encoding:** RFC 3986 boundary conditions, non-ASCII characters, reserved and unreserved sets.
- **HTML Sanitization:** Protection against HTML injection, script tags, entity escaping.
- **Markdown & Code Fences:** Multi-language fence extraction, nested backticks, raw code detection.
- **Shell Sanitization:** Fenced code stripping, whitespace normalization, multi-line command safety.
- **Config Persistence:** Auto-creation of missing configs, corrupted JSON recovery, environment variable overrides.
- **Context Sliding-Window Pruning:** Strict verification that message history never exceeds `history_limit`.
- **Session Persistence:** Full JSON round-trip serialization and deserialization across disk.
- **Client Factory:** Instantiation of all 7 engine clients with accurate fallback configurations.

---

## 🔧 How to Add a New AI Engine (Developer Guide)

Adding a new AI provider takes under 15 minutes by implementing the `IApiClient` interface:

1. **Create the header in `include/ulwai/MyEngineClient.hpp`:**
   ```cpp
   #pragma once
   #include "ulwai/IApiClient.hpp"

   namespace ulwai {
   class MyEngineClient : public IApiClient {
   public:
       explicit MyEngineClient(std::string apiKey, std::string model);
       std::string chat(const std::vector<ChatMessage>& messages,
                        const std::string& system_prompt = "",
                        StreamCallback onToken = nullptr) override;
       std::string name() const noexcept override { return "MyEngine"; }
   private:
       std::string apiKey_;
       std::string model_;
   };
   }
   ```
2. **Implement the network request in `src/MyEngineClient.cpp`:**
   Issue HTTP request via `libcurl` and invoke `onToken(delta)` as tokens arrive.
3. **Register in `src/ConversationManager.cpp`:**
   In `ConversationManager::createClient`:
   ```cpp
   if (eng == "myengine") {
       return std::make_unique<MyEngineClient>(config.myengine.api_key, config.myengine.model);
   }
   ```
4. **Add configuration field to `include/ulwai/Config.hpp` and build!**

---

## ❓ Troubleshooting & Frequently Asked Questions

#### Q: CMake says "Could not find a package configuration file provided by Qt6"
- **Solution:** Either install Qt6 (`sudo apt install qt6-base-dev` or `brew install qt@6`), or compile the pure terminal CLI version without Qt:
  ```bash
  cmake -B build -DBUILD_GUI=OFF
  cmake --build build
  ```

#### Q: How can I run ULWAI over SSH or in Docker without X11?
- **Solution:** ULWAI automatically detects headless environments. If `$DISPLAY` and `$WAYLAND_DISPLAY` are empty, running `./build/ulwai` defaults directly to terminal CLI mode. You can also run `./build/ulwai-cli` which has zero display server dependencies.

#### Q: "Connection refused" when querying Ollama
- **Solution:** Make sure the Ollama daemon is running locally (`ollama serve` or `systemctl start ollama`). Verify with `curl http://localhost:11434/api/tags`.

#### Q: How do I completely clear all saved conversations?
- **Solution:** Delete the history folder `rm -rf ~/.config/ulwai/history/` or open `Settings -> Storage & History -> Clear All History` in the desktop GUI.

---

## 🏷️ Versioning & Single Source of Truth

ULWAI adheres strictly to **Semantic Versioning (SemVer 2.0.0)**.
The project uses a single file as the single source of truth for the entire workspace:

- **[`VERSION`](VERSION)**: A plaintext file at the project root containing the current version string (e.g. `2.0.0`).

### How Version Propagation Works:
Whenever [`VERSION`](VERSION) is modified:
1. **CMake Build System:** [`CMakeLists.txt`](CMakeLists.txt) automatically detects that `VERSION` was modified (`CMAKE_CONFIGURE_DEPENDS`), reconfigures the build, updates `PROJECT_VERSION`, and defines `ULWAI_VERSION_STRING`.
2. **C++ Application Code:** [`include/ulwai/Version.hpp`](include/ulwai/Version.hpp) provides `ulwai::getVersion()`, which dynamically reflects in:
   - CLI command: `./build/ulwai --version`
   - CLI help banner: `./build/ulwai --help`
   - Interactive terminal REPL header: `./build/ulwai --cli`
   - Desktop GUI window title bar
   - Desktop GUI status bar
   - Desktop Settings modal
3. **Build & Test Scripts:** [`build.sh`](scripts/build.sh), [`build_all.py`](scripts/build_all.py), and [`run_tests.sh`](scripts/run_tests.sh) read [`VERSION`](VERSION) and print the version in their console banners.

**To release a new version:** Simply change the version number in [`VERSION`](VERSION) and compile!

---

## 🔒 Security & Privacy

- **Direct Connections Only:** All requests travel directly from your local machine to the provider's API endpoint via HTTPS. There are zero intermediary proxy servers or telemetry beacons.
- **No Hardcoded Credentials:** No keys or tokens are stored in the source code.
- **Interactive Shell Safety:** Shell Agent Mode (`-s`) will **never** execute an OS command without explicit user confirmation (`[y/N]`).
- **Open Source & Auditable:** All network callbacks and filesystem operations are fully transparent and documented in C++20.

---

## 👤 Author & Maintainer

- **Creator & Lead Maintainer:** **EG1 (Gautam Jangid)**
- **Official Portal:** [https://eg1.in](https://eg1.in)
- **GitHub Organization:** [@EG1DOTIN](https://github.com/EG1DOTIN)
- **Project Repository:** [https://github.com/EG1DOTIN/ulwai](https://github.com/EG1DOTIN/ulwai)

---

## 📄 License

This project is licensed under the terms of the **MIT License** — Copyright (c) 2026 EG1 (Gautam Jangid). See the [`LICENSE`](LICENSE) file for complete details.
