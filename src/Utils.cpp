/**
 * @file Utils.cpp
 * @brief Core String, Markdown, HTML, File I/O & Network Callback Utilities.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Contains shared, stateless helper functions utilized across both CLI and GUI subsystems.
 *   These routines handle text formatting, safe HTML generation, code fence extraction,
 *   terminal typewriter printing, and libcurl data transfer callbacks.
 *
 * KEY FUNCTIONAL MODULES:
 *   1. RFC 3986 URL Encoding (`urlEncode`):
 *      - Encodes query parameters safely for HTTP GET/POST requests.
 *   2. HTML Escaping & Sanitization (`escapeHtml`):
 *      - Replaces unsafe characters (`&`, `<`, `>`, `"`, `'`) with XML/HTML character entities
 *        to ensure robust rendering in Qt `QTextBrowser` without DOM injection vulnerabilities.
 *   3. Markdown to HTML Engine (`markdownToHtml`):
 *      - Converts CommonMark-style markdown headers, bold, italics, bullet lists, inline code,
 *        and triple-backtick fenced code blocks into formatted HTML.
 *      - Generates modern copy cards (`copy://<index>`) with language badges and one-click copy buttons.
 *   4. Shell Extraction (`stripShellMarkdown`):
 *      - Detects and unwraps bash/sh/zsh/fish/powershell command blocks emitted by AI models.
 *      - Cleanses shell commands for safe execution approval in `-s` / `--shell` mode.
 *   5. libcurl I/O (`curlWriteCallback`):
 *      - Standard C-compatible callback passed to `CURLOPT_WRITEFUNCTION` for string accumulation.
 *   6. Timestamping & File I/O:
 *      - ISO 8601 formatting for session storage and human-readable formatting for UI headers.
 *      - Safe file reading with bounds checks and exception handling.
 */

#include "ulwai/Utils.hpp"
#include "ulwai/Version.hpp"
#include "ulwai/CurlRaii.hpp"
#include "md4c/md4c.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <regex>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>

namespace ulwai::utils {

std::string urlEncode(std::string_view value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string escapeHtml(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 16);
    for (char c : text) {
        switch (c) {
            case '&':  out.append("&amp;"); break;
            case '<':  out.append("&lt;"); break;
            case '>':  out.append("&gt;"); break;
            case '"':  out.append("&quot;"); break;
            case '\'': out.append("&#39;"); break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

void escapeHtmlToStream(std::ostream& out, std::string_view text) {
    for (char c : text) {
        switch (c) {
            case '&':  out << "&amp;"; break;
            case '<':  out << "&lt;"; break;
            case '>':  out << "&gt;"; break;
            case '"':  out << "&quot;"; break;
            case '\'': out << "&#39;"; break;
            default:   out.put(c); break;
        }
    }
}

std::string stripShellMarkdown(std::string_view input) {
    std::string result(input);

    // Trim whitespace
    auto start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    result = result.substr(start);

    auto end = result.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) {
        result = result.substr(0, end + 1);
    }

    // Strip leading code fences ```bash / ```sh / ```
    if (result.starts_with("```")) {
        auto first_newline = result.find('\n');
        if (first_newline != std::string::npos) {
            result = result.substr(first_newline + 1);
        }
        if (result.ends_with("```")) {
            result = result.substr(0, result.size() - 3);
        }
        // Re-trim
        start = result.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        result = result.substr(start);
        end = result.find_last_not_of(" \t\n\r");
        if (end != std::string::npos) {
            result = result.substr(0, end + 1);
        }
    }

    return result;
}

std::vector<CodeBlock> extractCodeBlocks(std::string_view text) {
    std::vector<CodeBlock> blocks;
    if (text.empty()) return blocks;
    (void)markdownToHtml(text, 0, &blocks, false);
    return blocks;
}

std::string formatLanguageLabel(std::string_view lang) {
    if (lang.empty() || lang == "code" || lang == "Code") return "CODE";
    std::string s(lang);
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (s == "cpp" || s == "c++" || s == "cc" || s == "cxx" || s == "hpp" || s == "h") return "C++";
    if (s == "c") return "C";
    if (s == "arduino" || s == "ino") return "ARDUINO";
    if (s == "cs" || s == "csharp") return "C#";
    if (s == "py" || s == "python" || s == "py3") return "PYTHON";
    if (s == "js" || s == "javascript" || s == "jsx" || s == "node") return "JAVASCRIPT";
    if (s == "ts" || s == "typescript" || s == "tsx") return "TYPESCRIPT";
    if (s == "sh" || s == "bash" || s == "zsh" || s == "shell" || s == "ksh" || s == "terminal" || s == "console") return "BASH";
    if (s == "rs" || s == "rust") return "RUST";
    if (s == "golang" || s == "go") return "GO";
    if (s == "java") return "JAVA";
    if (s == "kt" || s == "kotlin") return "KOTLIN";
    if (s == "swift") return "SWIFT";
    if (s == "php") return "PHP";
    if (s == "rb" || s == "ruby") return "RUBY";
    if (s == "html" || s == "htm" || s == "xhtml" || s == "svg") return "HTML";
    if (s == "css" || s == "scss" || s == "sass" || s == "less") return "CSS";
    if (s == "json") return "JSON";
    if (s == "sql" || s == "mysql" || s == "pgsql" || s == "postgres" || s == "sqlite" || s == "tsql" || s == "plsql") return "SQL";
    if (s == "yaml" || s == "yml") return "YAML";
    if (s == "toml" || s == "ini") return "CONFIG";
    if (s == "xml") return "XML";
    if (s == "md" || s == "markdown") return "MARKDOWN";
    if (s == "docker" || s == "dockerfile") return "DOCKERFILE";
    if (s == "cmake") return "CMAKE";
    if (s == "make" || s == "makefile") return "MAKEFILE";
    if (s == "text" || s == "txt" || s == "plaintext" || s == "none") return "TEXT";

    std::string res(lang);
    for (char& c : res) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return res;
}

bool hasSignificantNaturalLanguageScript(std::string_view text) {
    size_t nonLatinCount = 0;
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    const size_t len = text.size();

    for (size_t i = 0; i < len;) {
        unsigned char c = bytes[i];
        if (c < 0x80) {
            i++;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < len) {
            uint32_t cp = ((c & 0x1F) << 6) | (bytes[i + 1] & 0x3F);
            // Arabic, Hebrew, Cyrillic
            if ((cp >= 0x0400 && cp <= 0x052F) ||
                (cp >= 0x0590 && cp <= 0x05FF) ||
                (cp >= 0x0600 && cp <= 0x06FF)) {
                nonLatinCount++;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < len) {
            uint32_t cp = ((c & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F);
            // Indic scripts (Devanagari U+0900..U+097F, Bengali, Gurmukhi, Gujarati, etc.: 0x0900..0x0DFF)
            if (cp >= 0x0900 && cp <= 0x0DFF) {
                nonLatinCount++;
            }
            // Japanese Kana & CJK: 0x3040..0x9FFF
            else if (cp >= 0x3040 && cp <= 0x9FFF) {
                nonLatinCount++;
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < len) {
            i += 4;
        } else {
            i++;
        }

        if (nonLatinCount >= 3) {
            return true;
        }
    }
    return false;
}

bool isLikelyPureCode(std::string_view text, std::string& detectedLang) {
    if (text.find("```") != std::string_view::npos || text.find("~~~") != std::string_view::npos) {
        return false;
    }

    if (hasSignificantNaturalLanguageScript(text)) {
        return false;
    }

    auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return false;
    std::string_view trimmed = text.substr(start);

    // Reject markdown quotes and prose structures (lists, bold markers, section headings)
    if (trimmed.starts_with("> ") ||
        text.find("\n### ") != std::string_view::npos ||
        text.find("\n## ") != std::string_view::npos ||
        text.find("\n1. ") != std::string_view::npos ||
        text.find("\n2. ") != std::string_view::npos ||
        text.find("\n- ") != std::string_view::npos ||
        text.find("\n* ") != std::string_view::npos ||
        text.find("**") != std::string_view::npos) {
        return false;
    }

    // 1. Shebang check (#!/...)
    if (trimmed.starts_with("#!/")) {
        if (trimmed.find("python") != std::string_view::npos) {
            detectedLang = "python";
        } else if (trimmed.find("bash") != std::string_view::npos || trimmed.find("sh") != std::string_view::npos || trimmed.find("zsh") != std::string_view::npos) {
            detectedLang = "bash";
        } else {
            detectedLang = "code";
        }
        return true;
    }

    // 2. HTML / XML (including HTML comments <!-- ... -->)
    if (trimmed.starts_with("<!DOCTYPE") || trimmed.starts_with("<html") || trimmed.starts_with("<?xml") ||
        (trimmed.starts_with("<!--") && (trimmed.find("<div") != std::string_view::npos || trimmed.find("<html") != std::string_view::npos || trimmed.find("</") != std::string_view::npos))) {
        detectedLang = "html";
        return true;
    }

    // 3. Multi-line scoring & language token analysis
    std::istringstream stream((std::string(text)));
    std::string line;
    size_t lineCount = 0;
    size_t commentLines = 0;
    size_t codeSyntaxLines = 0;

    int cppPoints = 0;
    int pyPoints = 0;
    int jsPoints = 0;
    int shPoints = 0;
    int sqlPoints = 0;
    int cssPoints = 0;
    int htmlPoints = 0;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto lstart = line.find_first_not_of(" \t");
        if (lstart == std::string::npos) continue;
        lineCount++;
        std::string_view lstr = std::string_view(line).substr(lstart);

        // Comment detection across languages
        if (lstr.starts_with("//") || lstr.starts_with("/*") || lstr.starts_with("* ") || lstr.starts_with("*/")) {
            commentLines++;
            cppPoints += 1;
            jsPoints += 1;
            cssPoints += 1;
            continue;
        } else if (lstr.starts_with("#")) {
            commentLines++;
            pyPoints += 1;
            shPoints += 1;
            continue;
        } else if (lstr.starts_with("--")) {
            commentLines++;
            sqlPoints += 2;
            continue;
        } else if (lstr.starts_with("<!--")) {
            commentLines++;
            htmlPoints += 2;
            continue;
        }

        // C / C++ / Arduino syntax & signatures
        if (lstr.starts_with("#include ") || lstr.starts_with("#define ") || lstr.starts_with("#ifdef ") ||
            lstr.starts_with("#ifndef ") || lstr.starts_with("#pragma ") || lstr.starts_with("using namespace ")) {
            cppPoints += 8;
            codeSyntaxLines++;
        }
        if (line.find("std::") != std::string::npos || line.find("cout") != std::string::npos ||
            line.find("nullptr") != std::string::npos || line.find("pinMode(") != std::string::npos ||
            line.find("digitalWrite(") != std::string::npos || line.find("digitalRead(") != std::string::npos ||
            line.find("analogRead(") != std::string::npos || line.find("analogWrite(") != std::string::npos ||
            line.find("Serial.") != std::string::npos || line.find("void setup()") != std::string::npos ||
            line.find("void loop()") != std::string::npos || line.find("int main(") != std::string::npos ||
            line.find("uint8_t") != std::string::npos || line.find("uint16_t") != std::string::npos ||
            line.find("uint32_t") != std::string::npos || line.find("size_t") != std::string::npos ||
            line.find("constexpr") != std::string::npos || line.find("noexcept") != std::string::npos ||
            line.find("const auto") != std::string::npos || line.find("malloc(") != std::string::npos ||
            line.find("printf(") != std::string::npos) {
            cppPoints += 5;
            codeSyntaxLines++;
        }

        // Python syntax & signatures
        if (lstr.starts_with("import ") || lstr.starts_with("from ") || lstr.starts_with("def ") ||
            lstr.starts_with("class ") || lstr.starts_with("async def ") || line.find("print(") != std::string::npos ||
            line.find("elif ") != std::string::npos || (line.find("return ") != std::string::npos && line.ends_with(":")) ||
            line.find("self.") != std::string::npos || line.find("__init__") != std::string::npos ||
            line.find("__name__") != std::string::npos || line.find("range(") != std::string::npos ||
            lstr.starts_with("with ") || (lstr.starts_with("except ") && line.ends_with(":"))) {
            pyPoints += 4;
            codeSyntaxLines++;
        }

        // JavaScript / TypeScript syntax
        if (lstr.starts_with("function ") || lstr.starts_with("const ") || lstr.starts_with("let ") ||
            lstr.starts_with("var ") || line.find("console.log(") != std::string::npos ||
            line.find("document.") != std::string::npos || line.find("window.") != std::string::npos ||
            line.find("=>") != std::string::npos || lstr.starts_with("export ") ||
            line.find("new Promise") != std::string::npos || lstr.starts_with("interface ") ||
            lstr.starts_with("type ") || line.find("require(") != std::string::npos) {
            jsPoints += 4;
            codeSyntaxLines++;
        }

        // Bash / Shell commands & syntax
        if (lstr.starts_with("echo ") || lstr.starts_with("export ") || lstr.starts_with("chmod ") ||
            lstr.starts_with("npm ") || lstr.starts_with("git ") || lstr.starts_with("sudo ") ||
            lstr.starts_with("curl ") || lstr.starts_with("mkdir ") || lstr.starts_with("cd ") ||
            lstr.starts_with("rm ") || lstr.starts_with("apt ") || lstr.starts_with("apt-get ") ||
            lstr.starts_with("pip install") || lstr.starts_with("systemctl ") || lstr.starts_with("cat ") ||
            lstr.starts_with("docker ") || line.find(" | ") != std::string::npos || line.find(" > ") != std::string::npos ||
            line.find(" 2>&1") != std::string::npos || lstr.starts_with("if [") || lstr == "fi" ||
            lstr == "done") {
            shPoints += 4;
            codeSyntaxLines++;
        }

        // SQL statements & keywords
        if (lstr.starts_with("SELECT ") || lstr.starts_with("select ") ||
            lstr.starts_with("INSERT INTO ") || lstr.starts_with("insert into ") ||
            lstr.starts_with("UPDATE ") || lstr.starts_with("update ") ||
            lstr.starts_with("DELETE FROM ") || lstr.starts_with("delete from ") ||
            lstr.starts_with("CREATE TABLE ") || lstr.starts_with("create table ") ||
            lstr.starts_with("ALTER TABLE ") || lstr.starts_with("alter table ") ||
            lstr.starts_with("DROP TABLE ") || lstr.starts_with("drop table ")) {
            sqlPoints += 6;
            codeSyntaxLines++;
        }
        if (line.find(" FROM ") != std::string::npos ||
            line.find(" WHERE ") != std::string::npos ||
            line.find(" JOIN ") != std::string::npos ||
            line.find(" VALUES ") != std::string::npos) {
            sqlPoints += 4;
            codeSyntaxLines++;
        }

        // HTML tags
        if (line.find("<div") != std::string::npos || line.find("</div>") != std::string::npos ||
            line.find("<span") != std::string::npos || line.find("<p>") != std::string::npos ||
            line.find("<a ") != std::string::npos || line.find("<table>") != std::string::npos ||
            line.find("<button") != std::string::npos || line.find("<input ") != std::string::npos) {
            htmlPoints += 4;
            codeSyntaxLines++;
        }

        // CSS rules
        if ((line.find("{") != std::string::npos && (line.find(".") != std::string::npos || line.find("#") != std::string::npos)) ||
            line.find("display:") != std::string::npos || line.find("color:") != std::string::npos ||
            line.find("background:") != std::string::npos || line.find("margin:") != std::string::npos ||
            line.find("padding:") != std::string::npos || line.find("font-size:") != std::string::npos ||
            line.find("border:") != std::string::npos) {
            cssPoints += 3;
            codeSyntaxLines++;
        }

        // General programming grammar (assignments, operators, semicolons, brackets)
        if (line.find(" = ") != std::string::npos || line.find(" == ") != std::string::npos ||
            line.find(" != ") != std::string::npos || line.find(" += ") != std::string::npos ||
            line.find(" -= ") != std::string::npos || line.find(" -> ") != std::string::npos ||
            (line.ends_with(";") && line.find("(") != std::string::npos) ||
            (line.ends_with("{") && line.find("(") != std::string::npos) ||
            line.find("for (") != std::string::npos || line.find("while (") != std::string::npos) {
            codeSyntaxLines++;
        }
    }

    if (lineCount == 0) return false;

    // Pick highest language score
    int maxPts = 0;
    std::string bestLang;
    auto checkLang = [&](int pts, const char* name) {
        if (pts > maxPts) { maxPts = pts; bestLang = name; }
    };
    checkLang(cppPoints, "cpp");
    checkLang(pyPoints, "python");
    checkLang(jsPoints, "javascript");
    checkLang(shPoints, "bash");
    checkLang(sqlPoints, "sql");
    checkLang(htmlPoints, "html");
    checkLang(cssPoints, "css");

    if (maxPts >= 3) {
        detectedLang = bestLang;
        return true;
    }

    // High general programming syntax density
    size_t nonCommentLines = (lineCount > commentLines) ? (lineCount - commentLines) : 1;
    if (codeSyntaxLines > 0 && (codeSyntaxLines * 100 / nonCommentLines) >= 50) {
        detectedLang = "code";
        return true;
    }

    return false;
}

size_t parseCopyBlockIndex(std::string_view url, bool& ok) {
    ok = false;
    std::string s(url);

    if (s.starts_with("copy://")) {
        s = s.substr(7);
    } else if (s.starts_with("copy:")) {
        s = s.substr(5);
    } else if (s.starts_with("copy-code://")) {
        s = s.substr(12);
    } else if (s.starts_with("copy-code:")) {
        s = s.substr(10);
    }

    while (!s.empty() && s.front() == '/') s.erase(s.begin());

    auto slashPos = s.find('/');
    std::string hostPart = (slashPos != std::string::npos) ? s.substr(0, slashPos) : s;

    // Check if hostPart is an IPv4 dotted quad string (e.g. QUrl turning copy://0 into 0.0.0.0)
    std::stringstream ss(hostPart);
    std::string seg;
    std::vector<unsigned int> octets;
    bool allOctetsOk = true;
    while (std::getline(ss, seg, '.')) {
        if (seg.empty() || seg.size() > 3) { allOctetsOk = false; break; }
        for (char c : seg) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { allOctetsOk = false; break; }
        }
        if (!allOctetsOk) break;
        unsigned int val = static_cast<unsigned int>(std::stoul(seg));
        if (val > 255) { allOctetsOk = false; break; }
        octets.push_back(val);
    }
    if (allOctetsOk && octets.size() == 4) {
        ok = true;
        return (static_cast<size_t>(octets[0]) << 24) |
               (static_cast<size_t>(octets[1]) << 16) |
               (static_cast<size_t>(octets[2]) << 8)  |
               static_cast<size_t>(octets[3]);
    }

    // Check if path follows slash (e.g. "block/0" or "block/10")
    if (slashPos != std::string::npos) {
        std::string pathPart = s.substr(slashPos + 1);
        while (!pathPart.empty() && pathPart.front() == '/') pathPart.erase(pathPart.begin());
        if (!pathPart.empty()) {
            bool allDigits = true;
            for (char c : pathPart) {
                if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
            }
            if (allDigits) {
                ok = true;
                return std::stoull(pathPart);
            }
        }
    }

    // Direct number e.g. "0" or "10"
    if (!hostPart.empty()) {
        bool allDigits = true;
        for (char c : hostPart) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
        }
        if (allDigits) {
            ok = true;
            return std::stoull(hostPart);
        }
    }

    // Fallback: extract trailing digits from entire string
    std::string tail;
    for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
        if (std::isdigit(static_cast<unsigned char>(s[i]))) {
            tail.insert(tail.begin(), s[i]);
        } else if (!tail.empty()) {
            break;
        }
    }
    if (!tail.empty()) {
        ok = true;
        return std::stoull(tail);
    }

    return 0;
}

static void appendCodeCardHtml(std::ostream& out, const std::string& langDisplay, size_t block_index, const std::string& code) {
    out << "<table class=\"code-card\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n"
        << "<tr>\n"
        << "<td class=\"code-header\">\n"
        << "<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n"
        << "<tr>\n"
        << "<td align=\"left\"><span class=\"code-lang\">";
    escapeHtmlToStream(out, langDisplay);
    out << "</span></td>\n"
        << "<td align=\"right\"><a href=\"copy://" << block_index << "\" class=\"copy-btn-link\">[📋 Copy code]</a></td>\n"
        << "</tr>\n"
        << "</table>\n"
        << "</td>\n"
        << "</tr>\n"
        << "<tr>\n"
        << "<td class=\"code-body-cell\">\n"
        << "<pre class=\"code-body\"><code>";
    escapeHtmlToStream(out, code);
    out << "</code></pre>\n"
        << "</td>\n"
        << "</tr>\n"
        << "</table>\n";
}

namespace {

struct MdRendererContext {
    std::ostringstream html;
    std::vector<CodeBlock>* outBlocks = nullptr;
    size_t code_index = 0;

    // Code block state
    bool in_code_block = false;
    std::string cur_code_lang;
    std::string cur_code_content;
};

static int on_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    auto* ctx = static_cast<MdRendererContext*>(userdata);

    switch (type) {
        case MD_BLOCK_DOC:
            break;
        case MD_BLOCK_P:
            ctx->html << "<p class=\"md-p\">";
            break;
        case MD_BLOCK_H: {
            auto* h = static_cast<MD_BLOCK_H_DETAIL*>(detail);
            ctx->html << "<h" << h->level << " class=\"md-h" << h->level << "\">";
            break;
        }
        case MD_BLOCK_QUOTE:
            ctx->html << "<blockquote class=\"md-quote\">\n";
            break;
        case MD_BLOCK_UL:
            ctx->html << "<ul class=\"md-ul\">\n";
            break;
        case MD_BLOCK_OL: {
            auto* ol = static_cast<MD_BLOCK_OL_DETAIL*>(detail);
            if (ol && ol->start != 1) {
                ctx->html << "<ol class=\"md-ol\" start=\"" << ol->start << "\">\n";
            } else {
                ctx->html << "<ol class=\"md-ol\">\n";
            }
            break;
        }
        case MD_BLOCK_LI: {
            auto* li = static_cast<MD_BLOCK_LI_DETAIL*>(detail);
            if (li && li->is_task) {
                if (li->task_mark == 'x' || li->task_mark == 'X') {
                    ctx->html << "<li class=\"md-li\"><span class=\"task-checkbox\">☑</span> ";
                } else {
                    ctx->html << "<li class=\"md-li\"><span class=\"task-checkbox\">☐</span> ";
                }
            } else {
                ctx->html << "<li class=\"md-li\">";
            }
            break;
        }
        case MD_BLOCK_HR:
            ctx->html << "<hr class=\"md-hr\"/>\n";
            break;
        case MD_BLOCK_TABLE:
            ctx->html << "<table class=\"md-table\">\n";
            break;
        case MD_BLOCK_THEAD:
            ctx->html << "<thead>\n";
            break;
        case MD_BLOCK_TBODY:
            ctx->html << "<tbody>\n";
            break;
        case MD_BLOCK_TR:
            ctx->html << "<tr>";
            break;
        case MD_BLOCK_TH: {
            auto* td = static_cast<MD_BLOCK_TD_DETAIL*>(detail);
            ctx->html << "<th";
            if (td) {
                if (td->align == MD_ALIGN_LEFT) ctx->html << " align=\"left\"";
                else if (td->align == MD_ALIGN_RIGHT) ctx->html << " align=\"right\"";
                else if (td->align == MD_ALIGN_CENTER) ctx->html << " align=\"center\"";
            }
            ctx->html << ">";
            break;
        }
        case MD_BLOCK_TD: {
            auto* td = static_cast<MD_BLOCK_TD_DETAIL*>(detail);
            ctx->html << "<td";
            if (td) {
                if (td->align == MD_ALIGN_LEFT) ctx->html << " align=\"left\"";
                else if (td->align == MD_ALIGN_RIGHT) ctx->html << " align=\"right\"";
                else if (td->align == MD_ALIGN_CENTER) ctx->html << " align=\"center\"";
            }
            ctx->html << ">";
            break;
        }
        case MD_BLOCK_CODE: {
            auto* code = static_cast<MD_BLOCK_CODE_DETAIL*>(detail);
            ctx->in_code_block = true;
            ctx->cur_code_content.clear();
            if (code && code->lang.text && code->lang.size > 0) {
                ctx->cur_code_lang = std::string(code->lang.text, code->lang.size);
                auto lstart = ctx->cur_code_lang.find_first_not_of(" \t\r\n");
                auto lend = ctx->cur_code_lang.find_last_not_of(" \t\r\n");
                if (lstart != std::string::npos && lend != std::string::npos) {
                    ctx->cur_code_lang = ctx->cur_code_lang.substr(lstart, lend - lstart + 1);
                }
            } else {
                ctx->cur_code_lang = "code";
            }
            break;
        }
        case MD_BLOCK_HTML:
        default:
            break;
    }
    return 0;
}

static int on_leave_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    auto* ctx = static_cast<MdRendererContext*>(userdata);

    switch (type) {
        case MD_BLOCK_DOC:
            break;
        case MD_BLOCK_P:
            ctx->html << "</p>\n";
            break;
        case MD_BLOCK_H: {
            auto* h = static_cast<MD_BLOCK_H_DETAIL*>(detail);
            ctx->html << "</h" << h->level << ">\n";
            break;
        }
        case MD_BLOCK_QUOTE:
            ctx->html << "</blockquote>\n";
            break;
        case MD_BLOCK_UL:
            ctx->html << "</ul>\n";
            break;
        case MD_BLOCK_OL:
            ctx->html << "</ol>\n";
            break;
        case MD_BLOCK_LI:
            ctx->html << "</li>\n";
            break;
        case MD_BLOCK_HR:
            break;
        case MD_BLOCK_TABLE:
            ctx->html << "</table>\n";
            break;
        case MD_BLOCK_THEAD:
            ctx->html << "</thead>\n";
            break;
        case MD_BLOCK_TBODY:
            ctx->html << "</tbody>\n";
            break;
        case MD_BLOCK_TR:
            ctx->html << "</tr>\n";
            break;
        case MD_BLOCK_TH:
            ctx->html << "</th>";
            break;
        case MD_BLOCK_TD:
            ctx->html << "</td>";
            break;
        case MD_BLOCK_CODE: {
            ctx->in_code_block = false;
            if (ctx->outBlocks) {
                CodeBlock cb;
                cb.index = ctx->code_index;
                cb.language = ctx->cur_code_lang;
                cb.code = ctx->cur_code_content;
                ctx->outBlocks->push_back(std::move(cb));
            }
            std::string langDisplay = formatLanguageLabel(ctx->cur_code_lang);
            appendCodeCardHtml(ctx->html, langDisplay, ctx->code_index++, ctx->cur_code_content);
            break;
        }
        case MD_BLOCK_HTML:
        default:
            break;
    }
    return 0;
}

static int on_enter_span(MD_SPANTYPE type, void* detail, void* userdata) {
    auto* ctx = static_cast<MdRendererContext*>(userdata);

    switch (type) {
        case MD_SPAN_EM:
            ctx->html << "<i>";
            break;
        case MD_SPAN_STRONG:
            ctx->html << "<b>";
            break;
        case MD_SPAN_A: {
            auto* a = static_cast<MD_SPAN_A_DETAIL*>(detail);
            ctx->html << "<a href=\"";
            if (a && a->href.text && a->href.size > 0) {
                escapeHtmlToStream(ctx->html, std::string_view(a->href.text, a->href.size));
            }
            ctx->html << "\" style=\"color: #38bdf8; text-decoration: underline;\">";
            break;
        }
        case MD_SPAN_IMG: {
            auto* img = static_cast<MD_SPAN_IMG_DETAIL*>(detail);
            ctx->html << "<img src=\"";
            if (img && img->src.text && img->src.size > 0) {
                escapeHtmlToStream(ctx->html, std::string_view(img->src.text, img->src.size));
            }
            ctx->html << "\" alt=\"";
            break;
        }
        case MD_SPAN_CODE:
            ctx->html << "<code class=\"inline-code\">";
            break;
        case MD_SPAN_DEL:
            ctx->html << "<s>";
            break;
        case MD_SPAN_U:
            ctx->html << "<u>";
            break;
        default:
            break;
    }
    return 0;
}

static int on_leave_span(MD_SPANTYPE type, void* detail, void* userdata) {
    (void)detail;
    auto* ctx = static_cast<MdRendererContext*>(userdata);

    switch (type) {
        case MD_SPAN_EM:
            ctx->html << "</i>";
            break;
        case MD_SPAN_STRONG:
            ctx->html << "</b>";
            break;
        case MD_SPAN_A:
            ctx->html << "</a>";
            break;
        case MD_SPAN_IMG:
            ctx->html << "\" style=\"max-width: 100%; height: auto; border-radius: 6px;\" />";
            break;
        case MD_SPAN_CODE:
            ctx->html << "</code>";
            break;
        case MD_SPAN_DEL:
            ctx->html << "</s>";
            break;
        case MD_SPAN_U:
            ctx->html << "</u>";
            break;
        default:
            break;
    }
    return 0;
}

static int on_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    auto* ctx = static_cast<MdRendererContext*>(userdata);

    if (ctx->in_code_block) {
        ctx->cur_code_content.append(text, size);
        return 0;
    }

    switch (type) {
        case MD_TEXT_NORMAL:
        case MD_TEXT_CODE:
        case MD_TEXT_LATEXMATH:
            escapeHtmlToStream(ctx->html, std::string_view(text, size));
            break;
        case MD_TEXT_NULLCHAR:
            break;
        case MD_TEXT_BR:
            ctx->html << "<br/>\n";
            break;
        case MD_TEXT_SOFTBR:
            ctx->html << "\n";
            break;
        case MD_TEXT_ENTITY:
            ctx->html.write(text, size);
            break;
        case MD_TEXT_HTML:
            escapeHtmlToStream(ctx->html, std::string_view(text, size));
            break;
    }
    return 0;
}

} // anonymous namespace

std::string markdownToHtml(std::string_view markdown, size_t start_code_index, std::vector<CodeBlock>* outBlocks, bool isCodeOnly) {
    if (markdown.empty()) {
        return "";
    }

    std::string detectedLang;
    bool isPureCode = false;

    // A whole message is ONLY treated as pure code if isCodeOnly is explicitly enabled (e.g. from Code Mode -c)
    // AND has no markdown fences AND passes pure code validation
    if (isCodeOnly &&
        markdown.find("```") == std::string_view::npos &&
        markdown.find("~~~") == std::string_view::npos &&
        !hasSignificantNaturalLanguageScript(markdown)) {
        if (isLikelyPureCode(markdown, detectedLang)) {
            isPureCode = true;
        } else if (!markdown.starts_with("#") &&
                   markdown.find("\n### ") == std::string_view::npos &&
                   markdown.find("\n## ") == std::string_view::npos &&
                   markdown.find("\n# ") == std::string_view::npos &&
                   markdown.find("\n- ") == std::string_view::npos &&
                   markdown.find("\n1. ") == std::string_view::npos) {
            isPureCode = true;
            detectedLang = "code";
        }
    }

    if (isPureCode) {
        if (detectedLang.empty()) detectedLang = "code";
        if (outBlocks) {
            CodeBlock cb;
            cb.index = start_code_index;
            cb.language = detectedLang;
            cb.code = std::string(markdown);
            outBlocks->push_back(std::move(cb));
        }
        std::string langDisplay = formatLanguageLabel(detectedLang);
        std::ostringstream out;
        appendCodeCardHtml(out, langDisplay, start_code_index, std::string(markdown));
        return out.str();
    }

    MdRendererContext ctx;
    ctx.outBlocks = outBlocks;
    ctx.code_index = start_code_index;

    MD_PARSER parser{};
    parser.abi_version = 0;
    parser.flags = MD_DIALECT_GITHUB | MD_FLAG_HARD_SOFT_BREAKS;
    parser.enter_block = on_enter_block;
    parser.leave_block = on_leave_block;
    parser.enter_span = on_enter_span;
    parser.leave_span = on_leave_span;
    parser.text = on_text;

    md_parse(markdown.data(), static_cast<MD_SIZE>(markdown.size()), &parser, &ctx);
    return ctx.html.str();
}

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}

std::string getCurrentTimestampIso() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string getCurrentTimeReadable() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool matchesTitle(std::string_view title, std::string_view query, const TitleSearchOptions& options) noexcept {
    if (query.empty()) {
        return true;
    }
    if (title.empty()) {
        return false;
    }

    if (options.use_regex) {
        try {
            auto flags = std::regex_constants::ECMAScript;
            if (!options.case_sensitive) {
                flags |= std::regex_constants::icase;
            }
            std::regex re(std::string(query), flags);
            return std::regex_search(std::string(title), re);
        } catch (...) {
            return false;
        }
    }

    if (options.match_whole_word) {
        auto isWordChar = [](char c) noexcept {
            return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
        };

        size_t pos = 0;
        while (pos < title.size()) {
            size_t matchPos = std::string_view::npos;
            if (options.case_sensitive) {
                matchPos = title.find(query, pos);
            } else {
                auto it = std::search(title.begin() + pos, title.end(), query.begin(), query.end(),
                    [](char a, char b) noexcept {
                        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                    });
                if (it != title.end()) {
                    matchPos = static_cast<size_t>(std::distance(title.begin(), it));
                }
            }

            if (matchPos == std::string_view::npos) {
                return false;
            }

            bool preBoundary = (matchPos == 0) || !isWordChar(title[matchPos - 1]);
            bool postBoundary = (matchPos + query.size() >= title.size()) || !isWordChar(title[matchPos + query.size()]);

            if (preBoundary && postBoundary) {
                return true;
            }

            pos = matchPos + 1;
        }
        return false;
    }

    if (options.case_sensitive) {
        return title.find(query) != std::string_view::npos;
    }

    auto it = std::search(title.begin(), title.end(), query.begin(), query.end(),
        [](char a, char b) noexcept {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        });
    return it != title.end();
}

void terminalTypingPrint(std::string_view text, unsigned int delayMs) {
    for (char c : text) {
        std::cout << c << std::flush;
        if (delayMs > 0 && (c == ' ' || c == '\n')) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }
    std::cout << std::endl;
}

size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

namespace {
std::tuple<int, int, int> parseSemver(std::string_view sv) noexcept {
    // Strip leading whitespace and 'v' / 'V'
    while (!sv.empty() && (std::isspace(static_cast<unsigned char>(sv.front())) || sv.front() == 'v' || sv.front() == 'V')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }

    int major = 0, minor = 0, patch = 0;
    int* parts[3] = {&major, &minor, &patch};
    int partIdx = 0;
    int currentVal = 0;
    bool inNumber = false;

    for (char c : sv) {
        if (c >= '0' && c <= '9') {
            currentVal = currentVal * 10 + (c - '0');
            inNumber = true;
        } else if (c == '.') {
            if (partIdx < 3) {
                *parts[partIdx++] = currentVal;
            }
            currentVal = 0;
            inNumber = false;
        } else {
            // Non-digit/dot (e.g. -alpha, -beta), stop numeric parsing
            break;
        }
    }
    if (inNumber && partIdx < 3) {
        *parts[partIdx] = currentVal;
    }

    return {major, minor, patch};
}
} // namespace

int compareSemanticVersions(std::string_view v1, std::string_view v2) noexcept {
    auto [maj1, min1, pat1] = parseSemver(v1);
    auto [maj2, min2, pat2] = parseSemver(v2);

    if (maj1 != maj2) return (maj1 < maj2) ? -1 : 1;
    if (min1 != min2) return (min1 < min2) ? -1 : 1;
    if (pat1 != pat2) return (pat1 < pat2) ? -1 : 1;
    return 0;
}

bool isNewerVersion(std::string_view currentVersion, std::string_view candidateVersion) noexcept {
    return compareSemanticVersions(currentVersion, candidateVersion) < 0;
}

UpdateCheckResult checkForUpdatesOnline(long timeoutSeconds) {
    UpdateCheckResult result;
    result.current_version = std::string(ulwai::getVersion());
    result.release_url = "https://github.com/EG1DOTIN/ulwai/releases/latest";

    ulwai::net::UniqueCurl curl(curl_easy_init());
    if (!curl) {
        result.error_message = "Failed to initialize network client";
        return result;
    }

    // 1. Primary check: Raw GitHub VERSION file (fast, reliable, zero rate limit)
    std::string rawUrl = "https://raw.githubusercontent.com/EG1DOTIN/ulwai/main/VERSION";
    std::string responseBody;
    long httpCode = 0;

    curl_easy_setopt(curl.get(), CURLOPT_URL, rawUrl.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, timeoutSeconds);
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, timeoutSeconds);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "ULWAI-Agent/2.0.0");

    CURLcode res = curl_easy_perform(curl.get());
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 200 && !responseBody.empty()) {
            size_t start = responseBody.find_first_not_of(" \t\r\n");
            size_t end = responseBody.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                std::string latestVer = responseBody.substr(start, end - start + 1);
                result.latest_version = latestVer;
                result.update_available = isNewerVersion(result.current_version, latestVer);
                result.success = true;
                return result;
            }
        }
    }

    // 2. Secondary fallback check: GitHub Releases API
    std::string apiUrl = "https://api.github.com/repos/EG1DOTIN/ulwai/releases/latest";
    responseBody.clear();
    curl_easy_setopt(curl.get(), CURLOPT_URL, apiUrl.c_str());

    ulwai::net::UniqueCurlSlist headers;
    ulwai::net::appendHeader(headers, "Accept: application/vnd.github.v3+json");
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());

    res = curl_easy_perform(curl.get());
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 200 && !responseBody.empty()) {
            try {
                auto j = nlohmann::json::parse(responseBody);
                if (j.contains("tag_name") && j["tag_name"].is_string()) {
                    std::string tagName = j["tag_name"].get<std::string>();
                    result.latest_version = tagName;
                    result.update_available = isNewerVersion(result.current_version, tagName);
                    if (j.contains("html_url") && j["html_url"].is_string()) {
                        result.release_url = j["html_url"].get<std::string>();
                    }
                    result.success = true;
                    return result;
                }
            } catch (...) {
                // Ignore parse errors, proceed to error diagnostics
            }
        }
    }

    result.success = false;
    if (res != CURLE_OK) {
        result.error_message = curl_easy_strerror(res);
    } else if (httpCode == 404) {
        result.error_message = "Remote repository not published or release not found (HTTP 404)";
    } else {
        result.error_message = "Server returned HTTP " + std::to_string(httpCode);
    }

    return result;
}

} // namespace ulwai::utils
