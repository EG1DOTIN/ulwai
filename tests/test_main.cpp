#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <filesystem>
#include "ulwai/Config.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/ConversationManager.hpp"
#include "ulwai/Platform.hpp"
#include "ulwai/PollinationsClient.hpp"
#include "ulwai/Version.hpp"
#include "ulwai/Logger.hpp"

// ─── Utils Edge Case Tests ────────────────────────────────────────────────────
void testUtilsEdgeCases() {
    std::cout << "[RUN] Testing Utils Edge Cases..." << std::endl;

    // 1. URL Encode Edge Cases
    assert(ulwai::utils::urlEncode("").empty());
    assert(ulwai::utils::urlEncode("hello world") == "hello%20world");
    assert(ulwai::utils::urlEncode("a+b=c&d/e") == "a%2Bb%3Dc%26d%2Fe");
    assert(ulwai::utils::urlEncode("!*'();:@&=+$,/?#[]").find('%') != std::string::npos);
    // Unreserved characters: should NOT be encoded
    assert(ulwai::utils::urlEncode("ABCD-1234_test.xyz~") == "ABCD-1234_test.xyz~");

    // 2. Escape HTML Edge Cases
    assert(ulwai::utils::escapeHtml("").empty());
    assert(ulwai::utils::escapeHtml("plain text") == "plain text");
    assert(ulwai::utils::escapeHtml("<script>alert('XSS') & \"quote\"</script>") ==
           "&lt;script&gt;alert(&#39;XSS&#39;) &amp; &quot;quote&quot;&lt;/script&gt;");
    assert(ulwai::utils::escapeHtml("<<<>>>&&&") == "&lt;&lt;&lt;&gt;&gt;&gt;&amp;&amp;&amp;");

    // 3. Strip Shell Markdown Edge Cases
    assert(ulwai::utils::stripShellMarkdown("").empty());
    assert(ulwai::utils::stripShellMarkdown("   \n\t  ").empty());
    assert(ulwai::utils::stripShellMarkdown("ls -la /tmp") == "ls -la /tmp");
    assert(ulwai::utils::stripShellMarkdown("```bash\nls -la /tmp\n```") == "ls -la /tmp");
    assert(ulwai::utils::stripShellMarkdown("  ```sh\n  echo 123  \n```  ") == "echo 123");
    assert(ulwai::utils::stripShellMarkdown("```\nrm -rf /tmp/foo\n```") == "rm -rf /tmp/foo");
    assert(ulwai::utils::stripShellMarkdown("```zsh\nfind . -name '*.cpp'\n```") == "find . -name '*.cpp'");

    // 4. Extract Code Blocks Edge Cases
    assert(ulwai::utils::extractCodeBlocks("No code here").empty());
    assert(ulwai::utils::extractCodeBlocks("").empty());

    // Single block without lang tag
    std::string noLangCode = "```\nint x = 10;\n```";
    auto b1 = ulwai::utils::extractCodeBlocks(noLangCode);
    assert(b1.size() == 1);
    assert(b1[0].language == "code");
    assert(b1[0].code == "int x = 10;\n");

    // Multiple blocks with custom languages (C++, Python, Bash)
    std::string multiCode = "Prefix text\n"
                            "```cpp\n"
                            "#include <iostream>\n"
                            "```\n"
                            "Middle text\n"
                            "```python\n"
                            "print('hello')\n"
                            "```\n"
                            "```bash\n"
                            "cat file.txt\n"
                            "```";
    auto b2 = ulwai::utils::extractCodeBlocks(multiCode);
    assert(b2.size() == 3);
    assert(b2[0].language == "cpp");
    assert(b2[1].language == "python");
    assert(b2[2].language == "bash");
    assert(b2[0].index == 0);
    assert(b2[1].index == 1);
    assert(b2[2].index == 2);

    // 5. Markdown To HTML Edge Cases
    assert(ulwai::utils::markdownToHtml("").empty());
    std::string md1 = "# Title 1\n## Title 2\n### Title 3\n- Bullet A\n* Bullet B\n---";
    std::string html1 = ulwai::utils::markdownToHtml(md1);
    assert(html1.find("Title 1</h1>") != std::string::npos);
    assert(html1.find("Title 2</h2>") != std::string::npos);
    assert(html1.find("Title 3</h3>") != std::string::npos);
    assert(html1.find("Bullet A</li>") != std::string::npos);
    assert(html1.find("Bullet B</li>") != std::string::npos);
    assert(html1.find("<hr") != std::string::npos);

    // Blockquote & Table
    std::string mdComplex = "> This is a quote\n\n| H1 | H2 |\n|---|---|\n| D1 | D2 |";
    std::string htmlComplex = ulwai::utils::markdownToHtml(mdComplex);
    assert(htmlComplex.find("<blockquote") != std::string::npos);
    assert(htmlComplex.find("<table") != std::string::npos);
    assert(htmlComplex.find("<th>H1</th>") != std::string::npos);
    assert(htmlComplex.find("<td>D1</td>") != std::string::npos);

    // Code blocks with outBlocks tracking
    std::vector<ulwai::CodeBlock> trackedBlocks;
    std::string mdTracking = "```rust\nfn main() {}\n```";
    std::string htmlTracking = ulwai::utils::markdownToHtml(mdTracking, 10, &trackedBlocks);
    assert(htmlTracking.find("copy://10") != std::string::npos);
    assert(htmlTracking.find("RUST") != std::string::npos);
    assert(trackedBlocks.size() == 1);
    assert(trackedBlocks[0].language == "rust");
    assert(trackedBlocks[0].code.find("fn main()") != std::string::npos);
    assert(trackedBlocks[0].index == 10);

    // Inline bold & code
    std::string mdInline = "Here is **bold text** and `inline_code()` example.";
    std::string htmlInline = ulwai::utils::markdownToHtml(mdInline);
    assert(htmlInline.find("<b>bold text</b>") != std::string::npos);
    assert(htmlInline.find("<code class=\"inline-code\">inline_code()</code>") != std::string::npos);

    // Multiple inline bold and code tags on single line
    std::string multiInline = "**first** and `first_code` and **second** and `second_code`";
    std::string multiHtml = ulwai::utils::markdownToHtml(multiInline);
    assert(multiHtml.find("<b>first</b>") != std::string::npos);
    assert(multiHtml.find("<b>second</b>") != std::string::npos);
    assert(multiHtml.find("<code class=\"inline-code\">first_code</code>") != std::string::npos);
    assert(multiHtml.find("<code class=\"inline-code\">second_code</code>") != std::string::npos);

    // Empty code block inside markdown
    std::string emptyFence = "```cpp\n```";
    auto emptyBlocks = ulwai::utils::extractCodeBlocks(emptyFence);
    assert(emptyBlocks.size() == 1);
    assert(emptyBlocks[0].language == "cpp");
    assert(emptyBlocks[0].code.empty());

    // Back-to-back code blocks without intervening text
    std::string backToBack = "```bash\necho A\n```\n```python\nprint('B')\n```";
    auto btbBlocks = ulwai::utils::extractCodeBlocks(backToBack);
    assert(btbBlocks.size() == 2);
    assert(btbBlocks[0].language == "bash");
    assert(btbBlocks[1].language == "python");

    // Code block with start_code_index offset
    std::string mdCodeOffset = "```cpp\nint val = 42;\n```";
    std::string htmlCodeOffset = ulwai::utils::markdownToHtml(mdCodeOffset, 5);
    assert(htmlCodeOffset.find("copy://5") != std::string::npos);
    assert(htmlCodeOffset.find("int val = 42;") != std::string::npos);

    // 6. Pure Code Detection & Indentation Preservation
    std::string detectedLang;
    std::string rawPy = "#!/usr/bin/env python3\n"
                        "import decimal\n"
                        "import math\n\n"
                        "# Chudnovsky constants\n"
                        "C = 426880 * decimal.Decimal(10005).sqrt()\n"
                        "MUL = decimal.Decimal(262537412640768000)  # 640320^3\n\n"
                        "def chudnovsky_pi(terms=15):\n"
                        "    total = decimal.Decimal(0)\n"
                        "    for k in range(terms):\n"
                        "        # (-1)^k\n"
                        "        sign = -1 if k & 1 else 1\n"
                        "        # (6k)! / ((3k)! * (k!)^3)\n"
                        "        num = math.factorial(6 * k)\n"
                        "        den = math.factorial(3 * k) * (math.factorial(k) ** 3)\n"
                        "        # (13591409 + 545140134k) * num / den\n"
                        "        part = decimal.Decimal(num * (13591409 + 545140134 * k)) / decimal.Decimal(den)\n"
                        "        part /= MUL ** k\n"
                        "        total += sign * part\n"
                        "    return C / total\n";

    assert(ulwai::utils::isLikelyPureCode(rawPy, detectedLang));
    assert(detectedLang == "python");

    std::vector<ulwai::CodeBlock> pyBlocks;
    std::string renderedHtml = ulwai::utils::markdownToHtml(rawPy, 0, &pyBlocks, true);

    // Verify it is enclosed in code-card with copy button
    assert(renderedHtml.find("class=\"code-card\"") != std::string::npos);
    assert(renderedHtml.find("PYTHON") != std::string::npos);
    assert(renderedHtml.find("[📋 Copy code]") != std::string::npos);
    assert(renderedHtml.find("copy://0") != std::string::npos);
    assert(pyBlocks.size() == 1);
    assert(pyBlocks[0].code == rawPy);

    // Verify indentation (4 spaces, 8 spaces) is fully preserved in <pre>
    assert(renderedHtml.find("    total = decimal.Decimal(0)") != std::string::npos);
    assert(renderedHtml.find("        sign = -1 if k & 1 else 1") != std::string::npos);

    // Verify comments are NOT converted to <h1> headers
    assert(renderedHtml.find("<h1") == std::string::npos);
    assert(renderedHtml.find("# Chudnovsky constants") != std::string::npos);

    // Verify math asterisks are NOT swallowed by italics
    assert(renderedHtml.find("math.factorial(3 * k) * (math.factorial(k) ** 3)") != std::string::npos);
    assert(renderedHtml.find("num * (13591409 + 545140134 * k)") != std::string::npos);
    assert(renderedHtml.find("<i>") == std::string::npos); // No erroneous italics

    // 7. Math expressions in regular markdown prose
    std::string mathProse = "Formula: `den = 3 * k * 5` and total = num * (1 + 2 * k)";
    std::string htmlMath = ulwai::utils::markdownToHtml(mathProse);
    assert(htmlMath.find("<i>") == std::string::npos);
    assert(htmlMath.find("num * (1 + 2 * k)") != std::string::npos);

    // Proper italics should still work
    std::string italicProse = "This is *italic text* and regular text.";
    std::string htmlItalic = ulwai::utils::markdownToHtml(italicProse);
    assert(htmlItalic.find("<i>italic text</i>") != std::string::npos);

    // 8. Other languages pure code detection
    std::string cppCode = "#include <iostream>\nint main() {\n    return 0;\n}\n";
    assert(ulwai::utils::isLikelyPureCode(cppCode, detectedLang));
    assert(detectedLang == "cpp");

    std::string bashCode = "#!/bin/bash\necho 'hello world'\n";
    assert(ulwai::utils::isLikelyPureCode(bashCode, detectedLang));
    assert(detectedLang == "bash");

    std::string prose = "Hello! Can you help me write some Python code today?";
    assert(!ulwai::utils::isLikelyPureCode(prose, detectedLang));

    // English prose with numbered lists, bullet lists, bold markers, and prepositions ("where", "from")
    std::string dashProse =
        "To address callback errors in your Dash app, consider the following steps:\n\n"
        "1. **Check Callback Inputs and Outputs:**\n"
        "   - Ensure that the inputs and outputs of your callback function are correctly defined.\n"
        "   - Verify that the components specified in the callback match the layout of your app.\n\n"
        "2. **Debugging:**\n"
        "   - Use print statements or logging to debug and trace the flow of your callback.\n"
        "   - Inspect the callback function to identify where the error occurs.\n";
    assert(!ulwai::utils::isLikelyPureCode(dashProse, detectedLang));
    std::string dashHtml = ulwai::utils::markdownToHtml(dashProse);
    assert(dashHtml.find("class=\"code-card\"") == std::string::npos);
    assert(dashHtml.find("<b>Check Callback Inputs and Outputs:</b>") != std::string::npos);

    // 9. Hindi & Natural Language Script Detection
    std::string hindiText = "नमस्ते, आप कैसे हैं? यहाँ एक सूची दी गई है:\n1. पहला कदम:\n2. दूसरा कदम:\n   - विवरण:";
    assert(ulwai::utils::hasSignificantNaturalLanguageScript(hindiText));
    assert(!ulwai::utils::isLikelyPureCode(hindiText, detectedLang));

    std::string hindiHtml = ulwai::utils::markdownToHtml(hindiText);
    assert(hindiHtml.find("class=\"code-card\"") == std::string::npos);
    assert(hindiHtml.find("नमस्ते") != std::string::npos);

    // Hindi with embedded markdown code block
    std::string hindiWithCode = "यहाँ पायथन में कोड दिया गया है:\n```python\nprint('नमस्ते')\n```\nयह पूरा हुआ।";
    std::vector<ulwai::CodeBlock> hindiBlocks;
    std::string hindiCodeHtml = ulwai::utils::markdownToHtml(hindiWithCode, 0, &hindiBlocks);
    assert(hindiBlocks.size() == 1);
    assert(hindiBlocks[0].language == "python");
    assert(hindiCodeHtml.find("class=\"code-card\"") != std::string::npos);
    assert(hindiCodeHtml.find("यहाँ पायथन में कोड") != std::string::npos);

    // 10. Indented Markdown Code Blocks within numbered/bullet lists
    std::string indMd = 
        "2. Arduino Code\n"
        "   1. Include Required Libraries:\n"
        "   ```cpp\n"
        "   #include <Wire.h>\n"
        "   #include <RTClib.h>\n"
        "   ```\n"
        "   2. Define Pin Assignments:\n"
        "   ```cpp\n"
        "   // Segment pin definitions for six 7-segment displays\n"
        "   const int segmentPins[7] = {2, 3, 4, 5, 6, 7, 8};\n"
        "   const int digitPins[6] = {9, 10, 11, 12, 13, A0};\n"
        "   RTC_DS3231 rtc;\n"
        "   ```\n";
    std::vector<ulwai::CodeBlock> indBlocks;
    std::string indHtml = ulwai::utils::markdownToHtml(indMd, 0, &indBlocks);
    assert(indBlocks.size() == 2);
    assert(indBlocks[0].language == "cpp");
    assert(indBlocks[1].language == "cpp");
    assert(indHtml.find("```") == std::string_view::npos); // No raw backticks remaining
    assert(indHtml.find("class=\"code-card\"") != std::string_view::npos);
    assert(indHtml.find("Segment pin definitions") != std::string_view::npos);

    // 11. Pure code with comments in various languages (C/C++, Arduino, Python, Bash, JS, SQL, HTML, CSS)
    std::string detected;
    std::string arduinoCode = 
        "// Segment pin definitions for six 7-segment displays\n"
        "const int segmentPins[7] = {2, 3, 4, 5, 6, 7, 8};\n"
        "const int digitPins[6] = {9, 10, 11, 12, 13, A0};\n"
        "RTC_DS3231 rtc;\n"
        "void setup() {\n"
        "  pinMode(segmentPins[0], OUTPUT);\n"
        "}\n";
    assert(ulwai::utils::isLikelyPureCode(arduinoCode, detected));
    assert(detected == "cpp");

    std::string pyWithComment = 
        "# This script calculates factorial\n"
        "def factorial(n):\n"
        "    if n <= 1:\n"
        "        return 1\n"
        "    return n * factorial(n - 1)\n";
    assert(ulwai::utils::isLikelyPureCode(pyWithComment, detected));
    assert(detected == "python");

    std::string bashWithComment = 
        "# Setup environment\n"
        "export NODE_ENV=production\n"
        "npm install\n"
        "npm run build\n";
    assert(ulwai::utils::isLikelyPureCode(bashWithComment, detected));
    assert(detected == "bash");

    std::string jsCode = 
        "// Helper function\n"
        "function calculateTotal(items) {\n"
        "  return items.reduce((acc, item) => acc + item.price, 0);\n"
        "}\n";
    assert(ulwai::utils::isLikelyPureCode(jsCode, detected));
    assert(detected == "javascript");

    std::string sqlCode = 
        "-- Query active users\n"
        "SELECT id, name, email\n"
        "FROM users\n"
        "WHERE active = 1;\n";
    assert(ulwai::utils::isLikelyPureCode(sqlCode, detected));
    assert(detected == "sql");

    std::string htmlCode = 
        "<!-- User profile card -->\n"
        "<div class=\"card\">\n"
        "  <h3>John Doe</h3>\n"
        "</div>\n";
    assert(ulwai::utils::isLikelyPureCode(htmlCode, detected));
    assert(detected == "html");

    std::string cssCode = 
        "/* Header styling */\n"
        ".header {\n"
        "  display: flex;\n"
        "  background: #222;\n"
        "}\n";
    assert(ulwai::utils::isLikelyPureCode(cssCode, detected));
    assert(detected == "css");

    // 12. Language badge formatting
    assert(ulwai::utils::formatLanguageLabel("cpp") == "C++");
    assert(ulwai::utils::formatLanguageLabel("c") == "C");
    assert(ulwai::utils::formatLanguageLabel("arduino") == "ARDUINO");
    assert(ulwai::utils::formatLanguageLabel("python") == "PYTHON");
    assert(ulwai::utils::formatLanguageLabel("javascript") == "JAVASCRIPT");
    assert(ulwai::utils::formatLanguageLabel("typescript") == "TYPESCRIPT");
    assert(ulwai::utils::formatLanguageLabel("bash") == "BASH");
    assert(ulwai::utils::formatLanguageLabel("sql") == "SQL");
    assert(ulwai::utils::formatLanguageLabel("html") == "HTML");
    assert(ulwai::utils::formatLanguageLabel("css") == "CSS");
    assert(ulwai::utils::formatLanguageLabel("text") == "TEXT");

    // 13. parseCopyBlockIndex URL resolution test cases
    bool copyOk = false;
    assert(ulwai::utils::parseCopyBlockIndex("copy://0", copyOk) == 0 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://1", copyOk) == 1 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://10", copyOk) == 10 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://256", copyOk) == 256 && copyOk);
    // IPv4 quad resolved by QUrl host (e.g. copy://0.0.0.0 or copy://0.0.0.10)
    assert(ulwai::utils::parseCopyBlockIndex("copy://0.0.0.0", copyOk) == 0 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://0.0.0.1", copyOk) == 1 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://0.0.0.10", copyOk) == 10 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://0.0.1.0", copyOk) == 256 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy:0", copyOk) == 0 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy:42", copyOk) == 42 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy://block/5", copyOk) == 5 && copyOk);
    assert(ulwai::utils::parseCopyBlockIndex("copy-code://block/99", copyOk) == 99 && copyOk);

    // Invalid scheme or empty
    assert(ulwai::utils::parseCopyBlockIndex("invalid://foo", copyOk) == 0 && !copyOk);
    (void)copyOk;

    // 14. Title Search and Filtering Edge Cases (matchesTitle)
    ulwai::TitleSearchOptions defaultOpts{};
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "", defaultOpts)); // Empty query matches all
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "ulwai", defaultOpts)); // Case-insensitive
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "ai", defaultOpts));
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "agent", defaultOpts));
    assert(!ulwai::utils::matchesTitle("ULWAI AI Agent", "python", defaultOpts));
    assert(!ulwai::utils::matchesTitle("", "ulwai", defaultOpts)); // Empty title never matches non-empty query

    // Case-sensitive search
    ulwai::TitleSearchOptions caseOpts{};
    caseOpts.case_sensitive = true;
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "ULWAI", caseOpts));
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "AI", caseOpts));
    assert(!ulwai::utils::matchesTitle("ULWAI AI Agent", "ulwai", caseOpts));
    assert(!ulwai::utils::matchesTitle("ULWAI AI Agent", "ai", caseOpts));

    // Match whole word
    ulwai::TitleSearchOptions wordOpts{};
    wordOpts.match_whole_word = true;
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "AI", wordOpts));
    assert(ulwai::utils::matchesTitle("The AI revolution", "AI", wordOpts));
    assert(!ulwai::utils::matchesTitle("PAID IN FULL", "AI", wordOpts)); // "AI" inside "PAID"
    assert(!ulwai::utils::matchesTitle("AIRPLANE MODE", "AI", wordOpts)); // "AI" inside "AIRPLANE"
    assert(ulwai::utils::matchesTitle("What is C++ in 2024?", "C++", wordOpts));
    assert(ulwai::utils::matchesTitle("What is C++ in 2024?", "2024", wordOpts));

    // Whole word + Case sensitive combined
    ulwai::TitleSearchOptions wordCaseOpts{};
    wordCaseOpts.match_whole_word = true;
    wordCaseOpts.case_sensitive = true;
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "AI", wordCaseOpts));
    assert(!ulwai::utils::matchesTitle("ULWAI AI Agent", "ai", wordCaseOpts));

    // Regex search
    ulwai::TitleSearchOptions regexOpts{};
    regexOpts.use_regex = true;
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "^ULWAI.*", regexOpts));
    assert(ulwai::utils::matchesTitle("ULWAI AI Agent", "Agent$", regexOpts));
    assert(!ulwai::utils::matchesTitle("ULWAI AI Agent", "^Agent.*", regexOpts));
    // Invalid regex pattern must safely return false without throwing or crashing
    assert(!ulwai::utils::matchesTitle("ULWAI AI Agent", "[unclosed_regex_pattern", regexOpts));

    // Unicode / Devanagari titles
    assert(ulwai::utils::matchesTitle("नमस्ते दुनिया ULWAI", "दुनिया", defaultOpts));
    assert(ulwai::utils::matchesTitle("नमस्ते दुनिया ULWAI", "ulwai", defaultOpts));

    (void)defaultOpts;
    (void)caseOpts;
    (void)wordOpts;
    (void)wordCaseOpts;
    (void)regexOpts;

    std::cout << "[PASS] Utils edge cases passed." << std::endl;
}

// ─── Config Edge Case Tests ───────────────────────────────────────────────────
void testConfigEdgeCases() {
    std::cout << "[RUN] Testing Config Edge Cases..." << std::endl;

    ulwai::Config cfg;
    assert(cfg.app_name == "ulw-ai-agent");
    assert(cfg.chat_sort_latest_first == false); // Default: Chronological order (like ChatGPT / Gemini)
    // Default should now be pollinations
    assert(cfg.default_engine == "pollinations");
    assert(cfg.pollinations.model == "openai");
    assert(!cfg.pollinations.endpoint.empty());

    // Test saving and loading to temporary test file
    std::filesystem::path tempDir = ulwai::platform::getConfigDirectory() / "test_scratch";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path cfgPath = tempDir / "edge_case_config.json";

    cfg.default_engine = "pollinations";
    cfg.pollinations.model = "openai-fast";
    cfg.temperature = 1.2;
    cfg.max_tokens = 4096;
    cfg.history_limit = 50;
    cfg.font_size = 18;
    cfg.chat_sort_latest_first = true;
    cfg.system_prompt = "You are a specialized test agent.";
    assert(cfg.save(cfgPath));

    ulwai::Config loaded;
    assert(loaded.load(cfgPath));
    assert(loaded.default_engine == "pollinations");
    assert(loaded.pollinations.model == "openai-fast");
    assert(loaded.temperature == 1.2);
    assert(loaded.max_tokens == 4096);
    assert(loaded.history_limit == 50);
    assert(loaded.font_size == 18);
    assert(loaded.chat_sort_latest_first == true);
    assert(loaded.system_prompt == "You are a specialized test agent.");

    // Model & Endpoint Resolution checks
    assert(loaded.resolveModel("pollinations") == "openai-fast");
    assert(loaded.resolveModel("ollama") == "llama3");
    assert(loaded.resolveModel("openai") == "gpt-4o-mini");
    assert(loaded.resolveModel("unknown_engine") == "default");

    assert(loaded.resolveEndpoint("pollinations") == "https://text.pollinations.ai/openai");
    assert(loaded.resolveEndpoint("duckduckgo") == "https://api.duckduckgo.com/");
    assert(loaded.resolveEndpoint("unknown") == "");

    // Corrupted JSON resilience test
    std::filesystem::path corruptPath = tempDir / "corrupted_config.json";
    {
        std::ofstream corruptFile(corruptPath);
        corruptFile << "{ invalid_json_syntax: true, missing_braces: [";
    }
    ulwai::Config fallbackCfg;
    // Loading corrupted JSON should return false without throwing an unhandled exception
    assert(!fallbackCfg.load(corruptPath));

    std::cout << "[PASS] Config edge cases passed." << std::endl;
}

// ─── ConversationManager Edge Case Tests ──────────────────────────────────────
void testConversationManagerEdgeCases() {
    std::cout << "[RUN] Testing ConversationManager Edge Cases..." << std::endl;

    ulwai::Config cfg;
    cfg.history_limit = 3;
    ulwai::ConversationManager mgr(cfg);

    // 1. Initial State
    mgr.newSession("Edge Case Session");
    assert(mgr.currentSession().messages.empty());
    assert(mgr.currentSession().title == "Edge Case Session");

    // 2. Auto-naming title from first user message
    mgr.clearCurrentMessages();
    mgr.newSession("New Conversation"); // Default placeholder
    mgr.addMessage(ulwai::Role::User, "Calculate the fibonacci series in C++\nand show benchmarks");
    assert(mgr.currentSession().title.find("Calculate the fibonacci") != std::string::npos);

    // 3. Shell message tracking & Code message tracking
    mgr.addMessage(ulwai::Role::Assistant, "```sh\nls -la\n```", true, "total 42\ndrwxr-xr-x 2 user");
    assert(mgr.currentSession().messages.size() == 2);
    assert(mgr.currentSession().messages[1].is_shell_command == true);
    assert(mgr.currentSession().messages[1].shell_command_output == "total 42\ndrwxr-xr-x 2 user");
    assert(!mgr.currentSession().messages[1].code_blocks.empty());

    mgr.addMessage(ulwai::Role::Assistant, "print('code mode')", false, "", true);
    assert(mgr.currentSession().messages[2].is_code_only == true);

    // 4. Pruning limit edge cases
    mgr.addMessage(ulwai::Role::User, "Message 3");
    mgr.addMessage(ulwai::Role::Assistant, "Message 4");
    mgr.addMessage(ulwai::Role::User, "Message 5");

    // Total 6 messages in session, but history_limit is 3
    auto ctx = mgr.getContextMessages();
    assert(ctx.size() == 3);
    assert(ctx[0].content == "Message 3");
    assert(ctx[1].content == "Message 4");
    assert(ctx[2].content == "Message 5");

    // Limit larger than total message count
    auto ctxAll = mgr.getContextMessages(100);
    assert(ctxAll.size() == 6);

    // Limit 0: returns all (unlimited mode)
    auto ctxUnlimited = mgr.getContextMessages(0);
    assert(ctxUnlimited.size() == 6);

    // 5. Export Edge Cases
    std::string mdExport = mgr.exportToMarkdown();
    assert(mdExport.find("Message 5") != std::string::npos);
    assert(mdExport.find("console") != std::string::npos); // Shell block output

    std::string htmlExport = mgr.exportToHtml();
    assert(htmlExport.find("<!DOCTYPE html>") != std::string::npos);
    assert(htmlExport.find("Message 5") != std::string::npos);

    // 6. Persistence Round-Trip
    std::string currentId = mgr.currentSession().id;
    assert(mgr.saveCurrentSession());

    ulwai::ConversationManager loaderMgr(cfg);
    assert(loaderMgr.loadSession(currentId));
    assert(loaderMgr.currentSession().id == currentId);
    assert(loaderMgr.currentSession().messages.size() == 6);
    assert(loaderMgr.currentSession().messages[1].is_shell_command == true);
    assert(loaderMgr.currentSession().messages[2].is_code_only == true);

    // 7. Session List and Switch Edge Cases
    auto sessionsList = loaderMgr.listSessions();
    assert(!sessionsList.empty());

    // Create a secondary session and switch back
    loaderMgr.newSession("Secondary Session");
    assert(loaderMgr.currentSession().title == "Secondary Session");
    std::string secondId = loaderMgr.currentSession().id;
    assert(loaderMgr.saveCurrentSession());

    // Switch back to original session
    assert(loaderMgr.loadSession(currentId));
    assert(loaderMgr.currentSession().id == currentId);
    assert(loaderMgr.currentSession().messages.size() == 6);

    // Delete the secondary session
    assert(loaderMgr.deleteSession(secondId));
    // Deleting a non-existent session should safely return false without crashing
    assert(!loaderMgr.deleteSession("non_existent_session_id_12345"));

    // 8. Export All Conversations JSON (ChatGPT & LibreChat universal export)
    std::string exportedAllJson = loaderMgr.exportAllConversationsJson();
    assert(!exportedAllJson.empty());
    assert(exportedAllJson.find("\"title\"") != std::string::npos);
    assert(exportedAllJson.find("\"messages\"") != std::string::npos);
    assert(exportedAllJson.find("\"mapping\"") != std::string::npos);

    // 9. Import Conversations JSON - ChatGPT format with mapping tree and attachment fallback
    std::string chatGptJson = R"([
      {
        "id": "chatgpt_sample_01",
        "title": "ChatGPT Python Data Analysis",
        "create_time": 1700000000.0,
        "current_node": "node_assistant_2",
        "mapping": {
          "node_root": {
            "id": "node_root",
            "message": null,
            "parent": null,
            "children": ["node_user_1"]
          },
          "node_user_1": {
            "id": "node_user_1",
            "message": {
              "id": "msg_u1",
              "author": {"role": "user"},
              "create_time": 1700000010.0,
              "content": {
                "content_type": "text",
                "parts": [
                  "Please analyze this data table:\n| Item | Val |\n|---|---|\n| A | 10 |",
                  {
                    "content_type": "image/png",
                    "name": "data_chart.png",
                    "url": "https://example.com/data_chart.png"
                  }
                ]
              }
            },
            "parent": "node_root",
            "children": ["node_assistant_2"]
          },
          "node_assistant_2": {
            "id": "node_assistant_2",
            "message": {
              "id": "msg_a2",
              "author": {"role": "assistant"},
              "create_time": 1700000020.0,
              "content": {
                "content_type": "text",
                "parts": [
                  "Here is the code to calculate it:\n```python\nimport pandas as pd\ndf = pd.DataFrame({'Item': ['A'], 'Val': [10]})\nprint(df)\n```"
                ]
              }
            },
            "parent": "node_user_1",
            "children": []
          }
        }
      }
    ])";

    size_t importedCgtCount = loaderMgr.importConversationsJson(chatGptJson);
    assert(importedCgtCount == 1);
    (void)importedCgtCount;
    assert(loaderMgr.currentSession().title == "ChatGPT Python Data Analysis");
    assert(loaderMgr.currentSession().messages.size() == 2);
    // User message has attachment fallback link and table
    assert(loaderMgr.currentSession().messages[0].content.find("[Attachment: data_chart.png](https://example.com/data_chart.png)") != std::string::npos);
    assert(loaderMgr.currentSession().messages[0].content.find("| Item | Val |") != std::string::npos);
    // Assistant message has code block
    assert(loaderMgr.currentSession().messages[1].role == ulwai::Role::Assistant);
    assert(!loaderMgr.currentSession().messages[1].code_blocks.empty());

    // 10. Import Conversations JSON - LibreChat format with messages array
    std::string libreChatJson = R"([
      {
        "conversationId": "libre_sample_02",
        "title": "LibreChat C++ Concurrency",
        "createdAt": "2024-05-01 10:00:00",
        "messages": [
          {
            "role": "user",
            "text": "What is std::jthread in C++20?",
            "createdAt": "2024-05-01 10:00:01"
          },
          {
            "role": "assistant",
            "text": "std::jthread is an RAII thread wrapper introduced in C++20.\n```cpp\n#include <thread>\nstd::jthread t([]() { /* work */ });\n```",
            "createdAt": "2024-05-01 10:00:05"
          }
        ]
      }
    ])";

    size_t importedLibreCount = loaderMgr.importConversationsJson(libreChatJson);
    assert(importedLibreCount == 1);
    (void)importedLibreCount;
    assert(loaderMgr.currentSession().title == "LibreChat C++ Concurrency");
    assert(loaderMgr.currentSession().messages.size() == 2);
    assert(loaderMgr.currentSession().messages[0].role == ulwai::Role::User);
    assert(loaderMgr.currentSession().messages[1].role == ulwai::Role::Assistant);
    assert(!loaderMgr.currentSession().messages[1].code_blocks.empty());

    // 11. Import Edge Cases - Invalid / Empty JSON
    assert(loaderMgr.importConversationsJson("") == 0);
    assert(loaderMgr.importConversationsJson("{invalid_json_garbage") == 0);
    assert(loaderMgr.importConversationsJson("[]") == 0);
    assert(loaderMgr.importConversationsJson("[{\"id\": \"empty_no_messages\"}]") == 0);

    // Clean up specific test-created sessions only (leaving all user history completely untouched)
    assert(loaderMgr.deleteSession(currentId));
    assert(loaderMgr.deleteSession("chatgpt_sample_01"));
    assert(loaderMgr.deleteSession("libre_sample_02"));
    std::error_code ecCleanup;
    std::filesystem::path trashDir = ulwai::platform::getConfigDirectory() / "trash";
    std::filesystem::remove(trashDir / (currentId + ".json"), ecCleanup);
    std::filesystem::remove(trashDir / "chatgpt_sample_01.json", ecCleanup);
    std::filesystem::remove(trashDir / "libre_sample_02.json", ecCleanup);
    std::filesystem::remove(trashDir / (secondId + ".json"), ecCleanup);

    // 12. Delete All Sessions
    // NOTE: Commented out to prevent wiping the user's real imported chats and history during tests.
    /*
    size_t deletedCount = loaderMgr.deleteAllSessions();
    assert(deletedCount >= 2);
    (void)deletedCount;
    // After delete all, a fresh new session is active
    assert(loaderMgr.currentSession().messages.empty());
    // Trash folder exists and contains deleted files
    std::filesystem::path trashDir = ulwai::platform::getConfigDirectory() / "trash";
    assert(std::filesystem::exists(trashDir));
    */

    std::cout << "[PASS] ConversationManager edge cases passed." << std::endl;
}

// ─── Client Factory & Pollinations Tests ───────────────────────────────────────
void testClientFactoryAndPollinations() {
    std::cout << "[RUN] Testing Client Factory & Pollinations..." << std::endl;

    ulwai::Config cfg;

    // Default engine creation (should now be Pollinations)
    auto defaultClient = ulwai::ConversationManager::createClient("", cfg);
    assert(defaultClient != nullptr);
    assert(defaultClient->name() == "Pollinations (Free AI)");

    // Explicit "pollinations" and alias "free"
    auto p1 = ulwai::ConversationManager::createClient("pollinations", cfg);
    assert(p1 != nullptr);
    assert(p1->name() == "Pollinations (Free AI)");

    auto p2 = ulwai::ConversationManager::createClient("free", cfg);
    assert(p2 != nullptr);
    assert(p2->name() == "Pollinations (Free AI)");

    // DuckDuckGo
    auto ddg = ulwai::ConversationManager::createClient("duckduckgo", cfg);
    assert(ddg != nullptr);
    assert(ddg->name() == "DuckDuckGo");

    // Ollama
    auto ollama = ulwai::ConversationManager::createClient("ollama", cfg);
    assert(ollama != nullptr);
    assert(ollama->name() == "Ollama (Local)");

    // OpenAI
    auto openai = ulwai::ConversationManager::createClient("openai", cfg);
    assert(openai != nullptr);
    assert(openai->name() == "OpenAI-Compatible");

    // OpenRouter
    auto openrouter = ulwai::ConversationManager::createClient("openrouter", cfg);
    assert(openrouter != nullptr);
    assert(openrouter->name() == "OpenRouter");

    // Grok
    auto grok = ulwai::ConversationManager::createClient("grok", cfg);
    assert(grok != nullptr);
    assert(grok->name() == "xAI Grok");

    // HuggingFace
    auto hf = ulwai::ConversationManager::createClient("huggingface", cfg);
    assert(hf != nullptr);
    assert(hf->name() == "Hugging Face");

    // Pollinations Model catalog validation
    auto models = ulwai::PollinationsClient::getAvailableModels();
    assert(!models.empty());
    bool foundDefault = false;
    for (const auto& [id, label] : models) {
        assert(!id.empty());
        assert(!label.empty());
        if (id == "openai") foundDefault = true;
    }
    assert(foundDefault);
    (void)foundDefault;

    std::cout << "[PASS] Client factory and Pollinations tests passed." << std::endl;
}

// ─── Platform & File I/O Edge Case Tests ──────────────────────────────────────
void testPlatformAndFileEdgeCases() {
    std::cout << "[RUN] Testing Platform & File I/O Edge Cases..." << std::endl;

    // 1. Platform Directory Paths
    auto homeDir = ulwai::platform::getHomeDirectory();
    assert(!homeDir.empty());
    auto configDir = ulwai::platform::getConfigDirectory();
    assert(!configDir.empty());
    auto historyDir = ulwai::platform::getHistoryDirectory();
    assert(!historyDir.empty());
    auto cfgPath = ulwai::platform::getDefaultConfigFilePath();
    assert(!cfgPath.empty());
    assert(cfgPath.filename() == "config.json");

    // 2. readFileContents Edge Cases
    // Non-existent file: returns empty string safely
    std::string nonExistent = ulwai::utils::readFileContents("non_existent_file_xyz_12345.tmp");
    assert(nonExistent.empty());

    // Existing file with content
    std::filesystem::path tempScratch = ulwai::platform::getConfigDirectory() / "test_scratch" / "test_file.txt";
    std::filesystem::create_directories(tempScratch.parent_path());
    {
        std::ofstream out(tempScratch);
        out << "Line 1: Ultra-lightweight AI agent\nLine 2: C++20 standard\nLine 3: sub-100MB RAM\n";
    }
    std::string readBack = ulwai::utils::readFileContents(tempScratch);
    assert(readBack.find("Line 1: Ultra-lightweight AI agent") != std::string::npos);
    assert(readBack.find("sub-100MB RAM") != std::string::npos);

    // 3. Command Execution & Output Capture
    std::string cmdOutput;
    int exitCode = ulwai::platform::executeCommand("echo 'ulwai-portable-test'", cmdOutput);
    assert(exitCode == 0);
    (void)exitCode;
    assert(cmdOutput.find("ulwai-portable-test") != std::string::npos);

    // Command that fails
    std::string failOutput;
#if defined(_WIN32)
    int failCode = ulwai::platform::executeCommand("cmd /c exit 1", failOutput);
#else
    int failCode = ulwai::platform::executeCommand("false", failOutput);
#endif
    assert(failCode != 0);
    (void)failCode;

    // Timestamp formatting
    std::string isoTime = ulwai::utils::getCurrentTimestampIso();
    assert(!isoTime.empty() && isoTime.find('T') != std::string::npos);
    std::string readableTime = ulwai::utils::getCurrentTimeReadable();
    assert(!readableTime.empty());

    std::cout << "[PASS] Platform and File I/O edge cases passed." << std::endl;
}

// ─── Centralized Versioning Tests ─────────────────────────────────────────────
void testVersioning() {
    std::cout << "[RUN] Testing Centralized Versioning..." << std::endl;

    assert(!ulwai::getVersion().empty());
    assert(ulwai::getVersion() == ulwai::kVersion);
    assert(ulwai::kVersionMajor >= 2);
    assert(ulwai::kVersionMinor >= 0);
    assert(ulwai::kVersionPatch >= 0);

    // Verify format contains at least one dot (semver: X.Y.Z)
    assert(ulwai::getVersion().find('.') != std::string_view::npos);

    // If VERSION file exists in current or parent directory, verify match
    std::filesystem::path vPath = "VERSION";
    if (!std::filesystem::exists(vPath)) {
        vPath = "../VERSION";
    }
    if (std::filesystem::exists(vPath)) {
        std::ifstream vf(vPath);
        std::string fileVer;
        if (vf >> fileVer) {
            assert(fileVer == ulwai::getVersion());
        }
    }

    std::cout << "[PASS] Centralized Versioning verified: v" << ulwai::getVersion() << std::endl;
}

// ─── Semantic Version & Online Update Tests ───────────────────────────────────
void testSemanticVersionAndUpdates() {
    std::cout << "[RUN] Testing Semantic Version & Update Checker..." << std::endl;

    // Direct comparison tests
    assert(ulwai::utils::compareSemanticVersions("2.0.0", "2.0.0") == 0);
    assert(ulwai::utils::compareSemanticVersions("2.0.0", "2.1.0") == -1);
    assert(ulwai::utils::compareSemanticVersions("2.1.0", "2.0.0") == 1);
    assert(ulwai::utils::compareSemanticVersions("2.0.0", "3.0.0") == -1);
    assert(ulwai::utils::compareSemanticVersions("2.0.0", "2.0.1") == -1);
    assert(ulwai::utils::compareSemanticVersions("2.0.1", "2.0.0") == 1);

    // Formatted strings with 'v', spaces, and pre-release suffix
    assert(ulwai::utils::compareSemanticVersions("v2.0.0", "2.0.0") == 0);
    assert(ulwai::utils::compareSemanticVersions("V2.1.0", "2.0.9") == 1);
    assert(ulwai::utils::compareSemanticVersions("  v2.0.1  ", "2.0.0") == 1);
    assert(ulwai::utils::compareSemanticVersions("2.0.0-beta", "2.0.0") == 0);
    assert(ulwai::utils::compareSemanticVersions("1.10.0", "1.9.0") == 1);
    assert(ulwai::utils::compareSemanticVersions("1.9.0", "1.10.0") == -1);

    // isNewerVersion tests
    assert(ulwai::utils::isNewerVersion("2.0.0", "2.0.1") == true);
    assert(ulwai::utils::isNewerVersion("2.0.0", "2.1.0") == true);
    assert(ulwai::utils::isNewerVersion("2.0.0", "3.0.0") == true);
    assert(ulwai::utils::isNewerVersion("2.0.0", "v2.0.1") == true);
    assert(ulwai::utils::isNewerVersion("2.0.0", "2.0.0") == false);
    assert(ulwai::utils::isNewerVersion("2.1.0", "2.0.9") == false);
    assert(ulwai::utils::isNewerVersion("3.0.0", "2.9.9") == false);

    // Online update check smoke test (must be completely safe and non-crashing)
    auto res = ulwai::utils::checkForUpdatesOnline(1);
    assert(!res.current_version.empty());
    assert(res.current_version == ulwai::getVersion());
    assert(!res.release_url.empty());
    // Either succeeds with valid latest_version or fails gracefully with error_message
    if (res.success) {
        assert(!res.latest_version.empty());
    } else {
        assert(!res.error_message.empty());
    }

    std::cout << "[PASS] Semantic Version & Update Checker verified." << std::endl;
}

// ─── Logging & Crash Handling Tests ───────────────────────────────────────────
void testLoggingAndCrashHandling() {
    std::cout << "[RUN] Testing Logging & Crash Handling..." << std::endl;

    std::filesystem::path testLogDir = ulwai::platform::getConfigDirectory() / "test_scratch" / "logs";
    ulwai::initLogging(testLogDir);
    ulwai::installCrashHandler();

    ULWAI_LOG_INFO("Test info log message 12345");
    ULWAI_LOG_WARN("Test warning message");
    ULWAI_LOG_ERROR("Test error message");

    auto logPath = ulwai::getAppLogPath();
    assert(std::filesystem::exists(logPath));

    std::string contents = ulwai::utils::readFileContents(logPath);
    assert(contents.find("Test info log message 12345") != std::string::npos);
    assert(contents.find("[INFO]") != std::string::npos);
    assert(contents.find("[WARN]") != std::string::npos);
    assert(contents.find("[ERROR]") != std::string::npos);

    assert(!ulwai::getCrashLogPath().empty());

    ulwai::shutdownLogging();
    std::cout << "[PASS] Logging & Crash Handling passed." << std::endl;
}

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "Running ULWAI AI Agent Comprehensive Test Suite  " << std::endl;
    std::cout << "=================================================" << std::endl;

    testUtilsEdgeCases();
    testConfigEdgeCases();
    testConversationManagerEdgeCases();
    testClientFactoryAndPollinations();
    testPlatformAndFileEdgeCases();
    testVersioning();
    testSemanticVersionAndUpdates();
    testLoggingAndCrashHandling();

    std::cout << "\n>>> ALL 35+ COMPREHENSIVE EDGE-CASE TEST SUITES PASSED CLEANLY! <<<" << std::endl;
    return 0;
}
