# ULWAI AI Agent — Query Flow & Streaming Architecture

> **Complete architectural execution flows for UI background threads, CLI token streaming, and safe shell execution.**

---

## 1. Desktop UI Asynchronous Query & Token Streaming

In Desktop UI mode, responsiveness is maintained by offloading network I/O and SSE parsing to a dedicated background worker thread (`AiWorker`, subclassing `QThread`). This prevents the Qt GUI event loop from freezing, ensuring silky 60 FPS UI performance even during long generational queries.

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant Main as MainWindow (UI Thread)
    participant View as ChatBubbleView
    participant Worker as AiWorker (Background QThread)
    participant Client as IApiClient (e.g. OpenAIClient)
    participant Server as Cloud/Local LLM Endpoint

    User->>Main: Submits prompt (Enter or Send Button)
    Main->>View: Append user message bubble & start streaming placeholder
    Main->>Worker: Instantiate & call start() with active context
    Worker->>Client: chat(messages, system_prompt, onTokenCallback)
    Client->>Server: HTTP POST request (stream=true, headers, json body)

    loop Server-Sent Events (SSE) Stream
        Server-->>Client: data: {"choices":[{"delta":{"content":"token"}}]}
        Client->>Worker: onTokenCallback("token")
        Worker->>Main: emit tokenReceived("token") (Qt Signal/Slot across threads)
        Main->>View: appendStreamingToken("token") & auto-scroll
    end

    Server-->>Client: data: [DONE] / Connection closed
    Client-->>Worker: Full accumulated response string
    Worker->>Main: emit finished(fullText, error)
    Main->>View: finishStreaming(fullText) & update session
    Main->>Main: Auto-save session to ~/.config/ulwai/history/
```

### Key Implementation Details:
1. **Thread Boundary Safety:** `AiWorker` never touches Qt UI widgets directly. All communication is conducted via Qt's queued signals and slots mechanism (`tokenReceived`, `finished`), which automatically marshals data onto the main UI thread.
2. **Cancellation Support:** When the user clicks `Stop`, `AiWorker::cancel()` sets an atomic flag (`cancelled_`), which terminates the libcurl transfer loop cleanly.

---

## 2. Terminal CLI Streaming & Typewriter Flow

In CLI mode, ULWAI operates with zero Qt dependencies. The streaming flow directly binds libcurl data transfer chunks to `stdout`:

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant CLI as CliRunner
    participant Mgr as ConversationManager
    participant Client as IApiClient
    participant Server as AI Server Endpoint

    User->>CLI: Invokes: ulwai "Explain C++20 concepts"
    CLI->>Mgr: Prepare context & active history
    CLI->>Client: chatSingle(prompt, system_prompt, onToken)
    Client->>Server: HTTP POST (stream=true)
    loop Data Chunks Received
        Server-->>Client: Raw bytes chunk
        Client->>CLI: onToken(delta)
        CLI->>CLI: Print delta directly to stdout with ANSI formatting
    end
    Client-->>CLI: Return full accumulated response
    CLI->>Mgr: Append assistant turn to history
    CLI->>Mgr: saveCurrentSession() to disk
```

---

## 3. Shell Command Agent Mode (`-s`) Safety Flow

When operating in **Shell Mode** (`-s` or `--shell`), ULWAI guarantees that **no command runs without explicit interactive user approval**.

```mermaid
flowchart TD
    Start["User issues goal (e.g. 'clean docker images older than 30 days')"] --> Context["Inject Shell Expert System Prompt"]
    Context --> Request["Send request to AI Engine"]
    Request --> Generate["AI Generates Shell Command"]
    Generate --> Parse["stripShellMarkdown() extracts clean executable command"]

    Parse --> UIBranch{"Is GUI or CLI Active?"}

    UIBranch -->|GUI Mode| ModalDialog["Pop up interactive Shell Confirmation Dialog"]
    UIBranch -->|CLI Mode| TerminalPrompt["Print cyan command block & prompt [y/N]"]

    ModalDialog --> UserDecision{"User Approves?"}
    TerminalPrompt --> UserDecision

    UserDecision -->|Approved| Exec["platform::executeCommand(cmd, output)"]
    UserDecision -->|Rejected / Cancelled| Skip["Log 'Command cancelled by user'"]

    Exec --> Pipe["Spawn subshell via popen(fullCmd + ' 2>&1')"]
    Pipe --> Capture["Stream stdout & stderr into output buffer"]
    Capture --> Feedback["Append command output back into conversation memory"]
    Skip --> End["Done"]
    Feedback --> End
```

### Safety Guarantees:
- **Explicit User Gate:** No automated subshell execution can bypass the confirmation prompt.
- **Output Re-ingestion:** The command's stdout and stderr are fed back into the multi-turn context, allowing the user to ask follow-up questions such as *"Why did that command fail?"* or *"Analyze this output"*.
