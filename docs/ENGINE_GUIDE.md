# ULWAI AI Agent — Engine & Provider Deep-Dive Guide

> **Comprehensive reference, configuration steps, and developer guide for all 7 supported AI backends.**

ULWAI's polymorphic architecture abstracts all AI providers behind the clean `IApiClient` C++20 interface. This document provides step-by-step "how to" configuration instructions, model catalogs, performance characteristics, and code references for every supported engine.

---

## 1. Engine Comparison Matrix

| Engine | Service | Requires Key? | Streaming Protocol | Default Model | Best For |
| :--- | :--- | :---: | :---: | :--- | :--- |
| **`pollinations`** | [Pollinations.ai](https://pollinations.ai) | ❌ **No (Free)** | Server-Sent Events (SSE) | `openai` (GPT-OSS 20B) | Instant conversational chat & code generation out-of-the-box. |
| **`duckduckgo`** | DuckDuckGo Instant Answer | ❌ **No (Free)** | Word Burst Simulation | `default` | Quick factual lookups, Wikipedia summaries, unit & currency conversions. |
| **`ollama`** | Local Ollama / llama.cpp | ❌ **No (Free)** | Newline-Delimited JSON (NDJSON) | `llama3` | 100% private, offline inference on local CPU/GPU hardware. |
| **`openai`** | OpenAI / OpenAI-Compatible | ✔ Yes (or local server) | Server-Sent Events (SSE) | `gpt-4o-mini` | Production-grade reasoning, large context windows, custom endpoints (LM Studio, vLLM). |
| **`openrouter`** | OpenRouter Gateway | ✔ Yes (Free tier available) | Server-Sent Events (SSE) | `meta-llama/llama-3.1-8b-instruct:free` | Aggregator for Claude 3.5, Gemini, Llama 3, DeepSeek, and free models. |
| **`grok`** | xAI Grok API | ✔ Yes | Server-Sent Events (SSE) | `grok-beta` | High-accuracy real-time reasoning and code generation. |
| **`huggingface`** | Hugging Face Hub | ✔ Yes | Word Burst Simulation | `mistralai/Mistral-7B-Instruct-v0.2` | Open-source research models hosted on Hugging Face Serverless API. |

---

## 2. In-Depth Guide per Engine

### A. Pollinations.ai (Default Free Engine)
Pollinations provides public, community-funded OpenAI-compatible endpoints with zero fees, zero account sign-up, and zero API keys.

#### How It Works:
- ULWAI sends an HTTP POST request formatted to OpenAI specification (`/v1/chat/completions`) to `https://text.pollinations.ai/openai`.
- Real-time token streaming is parsed live from the SSE event stream (`data: {"choices":[{"delta":{"content":"..."}}]}`).

#### Available Models:
- `openai`: GPT-OSS 20B reasoning architecture. High general capability (default).
- `openai-fast`: Low-latency variant optimized for rapid interactive responses.
- `gpt-oss`: Pure open-source transformer architecture.

#### How to Use:
```bash
# Default query
./build/ulwai "Explain how virtual memory works"

# Fast reasoning model
./build/ulwai -m pollinations --model openai-fast "Write a quicksort in C++"
```

---

### B. DuckDuckGo Instant Answer
DuckDuckGo provides free computational and informational queries without tracking or authentication.

#### How It Works:
1. **Local Heuristic Evaluation:** ULWAI first evaluates standard mathematical expressions and unit conversions (e.g. miles to kilometers, Fahrenheit to Celsius) with high-speed regex routines in `DuckDuckGoClient.cpp` to deliver sub-millisecond offline answers.
2. **HTTP API Request:** For general questions, ULWAI queries `https://api.duckduckgo.com/?q=<encoded>&format=json&no_html=1`.
3. **Structured Field Extraction:** Parses JSON priority fields: `Answer`, `AbstractText`, `Definition`, and `RelatedTopics`.

#### How to Use:
```bash
# Unit conversions
./build/ulwai -m duckduckgo "100 miles in km"

# Wikipedia definitions
./build/ulwai -m duckduckgo "Alan Turing"
```

---

### C. Ollama / llama.cpp (Offline Privacy)
Runs entirely on your local workstation with zero telemetry and zero external network calls.

#### Step-by-Step Setup:
1. **Install Ollama:**
   ```bash
   curl -fsSL https://ollama.com/install.sh | sh
   ```
2. **Pull Your Preferred Model:**
   ```bash
   ollama pull llama3
   # or coding-specific models:
   ollama pull deepseek-coder:6.7b
   ollama pull qwen2.5-coder:7b
   ```
3. **Query via ULWAI:**
   ```bash
   ./build/ulwai -m ollama --model llama3 "Explain RAII in C++"
   ```

#### Custom Ollama Host or Port:
If Ollama is running on a remote LAN server or custom port, export `$OLLAMA_HOST`:
```bash
export OLLAMA_HOST="http://192.168.1.50:11434"
./build/ulwai -m ollama "Hello from LAN Ollama"
```

---

### D. OpenAI & OpenAI-Compatible Servers
Connects to official OpenAI models or any self-hosted inference server implementing the OpenAI Chat Completions API standard.

#### Step-by-Step Setup for Official OpenAI:
1. Obtain an API key from [platform.openai.com](https://platform.openai.com/api-keys).
2. Set your environment variable:
   ```bash
   export OPENAI_API_KEY="sk-..."
   ```
3. Query OpenAI:
   ```bash
   ./build/ulwai -m openai --model gpt-4o-mini "Write a thread-safe circular buffer in C++20"
   ```

#### Connecting to Local Self-Hosted Servers:
ULWAI works out-of-the-box with self-hosted OpenAI-compatible servers:
- **LM Studio:** Set endpoint to `http://localhost:1234/v1/chat/completions`.
- **vLLM:** Set endpoint to `http://localhost:8000/v1/chat/completions`.
- **llama.cpp server:** Set endpoint to `http://localhost:8080/v1/chat/completions`.
- **LocalAI:** Set endpoint to `http://localhost:8080/v1/chat/completions`.

You can update these in `~/.config/ulwai/config.json`:
```json
"openai": {
  "endpoint": "http://localhost:1234/v1/chat/completions",
  "model": "local-model"
}
```

---

### E. OpenRouter (Multi-Model Gateway)
Provides unified access to hundreds of proprietary and open-source models through a single API key.

#### Step-by-Step Setup:
1. Get an API key from [openrouter.ai/keys](https://openrouter.ai/keys).
2. Export your key:
   ```bash
   export OPENROUTER_API_KEY="sk-or-..."
   ```
3. Query any model:
   ```bash
   # Free model
   ./build/ulwai -m openrouter --model "meta-llama/llama-3.1-8b-instruct:free" "Explain Coroutines"

   # Anthropic Claude 3.5 Sonnet
   ./build/ulwai -m openrouter --model "anthropic/claude-3.5-sonnet" "Audit this system architecture"
   ```

---

### F. xAI Grok
Native support for xAI's frontier models.

#### Step-by-Step Setup:
1. Create an API key at [console.x.ai](https://console.x.ai).
2. Set your environment variable:
   ```bash
   export XAI_API_KEY="xai-..."
   ```
3. Query Grok:
   ```bash
   ./build/ulwai -m grok --model grok-beta "Explain general relativity and spacetime curvature"
   ```

---

### G. Hugging Face Serverless Inference API
Queries models hosted on the Hugging Face Hub using serverless inference.

#### Step-by-Step Setup:
1. Generate an Access Token with read permissions at [huggingface.co/settings/tokens](https://huggingface.co/settings/tokens).
2. Export the token:
   ```bash
   export HF_TOKEN="hf_..."
   ```
3. Query a Hugging Face model:
   ```bash
   ./build/ulwai -m huggingface --model "mistralai/Mistral-7B-Instruct-v0.2" "Hello Mistral"
   ```

---

## 3. Polymorphic Architecture & Adding a New Engine

All engine clients inherit from `IApiClient` ([`include/ulwai/IApiClient.hpp`](../include/ulwai/IApiClient.hpp)):

```cpp
namespace ulwai {

class IApiClient {
public:
    virtual ~IApiClient() = default;

    [[nodiscard]] virtual std::string chat(
        const std::vector<ChatMessage>& messages,
        const std::string& system_prompt = "",
        StreamCallback onToken = nullptr) = 0;

    [[nodiscard]] virtual std::string name() const noexcept = 0;
};

} // namespace ulwai
```

### How to Add an 8th Engine:
1. **Create the Client Class:**
   Create `include/ulwai/CustomEngineClient.hpp` and `src/CustomEngineClient.cpp`. Inherit from `IApiClient` (or `OpenAIClient` if the provider adheres to the OpenAI spec).
2. **Implement Network Streaming:**
   Configure `libcurl` options (`CURLOPT_URL`, `CURLOPT_WRITEFUNCTION`, `CURLOPT_WRITEDATA`, `CURLOPT_HTTPHEADER`).
3. **Register in Factory:**
   Add a case to `ConversationManager::createClient()` in [`src/ConversationManager.cpp`](../src/ConversationManager.cpp):
   ```cpp
   if (eng == "custom") {
       return std::make_unique<CustomEngineClient>(config.custom.api_key, config.custom.model);
   }
   ```
4. **Add to Settings UI & Config:**
   Update `Config.hpp` to store the credentials and add the engine to `SettingsDialog.cpp`.
