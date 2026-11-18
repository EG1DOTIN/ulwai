# Third-Party Libraries and Dependencies

ULWAI AI Agent adheres to a strict zero-bloat philosophy. External dependencies are kept to an absolute minimum to ensure:
- Sub-100 MB RAM usage in GUI mode and sub-15 MB RAM in CLI mode.
- Fast, reproducible, zero-setup builds across Linux, Windows, and macOS.
- Independent operation of the terminal client (`ulwai-cli`) with zero GUI/Qt dependencies.

Below is the complete inventory of third-party libraries and tools used in ULWAI:

---

## 1. MD4C (Markdown for C)
- **Source Repository:** [github.com/mity/md4c](https://github.com/mity/md4c)
- **License:** MIT License
- **Author:** Martin Mitáš
- **Location:** `deps/md4c/` (`md4c.h`, `md4c.c`, `LICENSE.md`)
- **Role in ULWAI:**
  - Fast, zero-copy, SAX-style streaming parser for CommonMark and GitHub Flavored Markdown (GFM).
  - Used in `src/Utils.cpp` (`markdownToHtml` and `extractCodeBlocks`) for parsing markdown documents, lists, tables, blockquotes, inline formatting, and fenced code blocks.
  - Powers interactive code cards with language badges and `[📋 Copy code]` buttons without regex heuristics or layout glitches.
  - Operates identically in both desktop GUI (`ulwai-gui`) and standalone CLI (`ulwai-cli`).

---

## 2. nlohmann/json
- **Source Repository:** [github.com/nlohmann/json](https://github.com/nlohmann/json)
- **License:** MIT License
- **Author:** Niels Lohmann and contributors
- **Location:** `deps/nlohmann/`
- **Role in ULWAI:**
  - Modern, header-only C++ JSON parser and serializer.
  - Powers configuration loading/saving (`~/.config/ulwai/config.json`).
  - Implements the file-based conversation history store (`~/.config/ulwai/history/*.json`).
  - Formats API query payloads and parses responses across all supported AI engines (OpenAI, Ollama, OpenRouter, Pollinations, Grok, Hugging Face).

---

## 3. libcurl
- **Source Repository:** [curl.se](https://curl.se) / [github.com/curl/curl](https://github.com/curl/curl)
- **License:** curl License (MIT/X-style open source license)
- **Author:** Daniel Stenberg and contributors
- **Location:** System library dynamically linked (`libcurl.so`), headers in `deps/curl/include`
- **Role in ULWAI:**
  - Industrial-strength, cross-platform HTTP/HTTPS network transport engine.
  - Handles streaming and buffered API queries, SSL verification, timeout management, and network error reporting.

---

## 4. Qt 6 (Optional Desktop GUI Subsystem)
- **Source Repository:** [qt.io](https://www.qt.io)
- **License:** LGPLv3 (GNU Lesser General Public License - 100% Free & Open Source)
- **Author:** The Qt Company and contributors
- **Role in ULWAI:**
  - Powers the desktop graphical user interface executable (`ulwai` / `ulw-ai-agent`).
  - Provides rich text rendering (`QTextBrowser`), dark theme styling (`QSS`), session sidebar (`QListWidget`), and modal settings (`QDialog`).
  - **Licensing Compliance:** ULWAI links dynamically against system or official Qt libraries under the free and open-source **LGPLv3**. No commercial license, paid agreement, or proprietary Qt fee is used or required.
  - **Zero-GUI Modularity:** The standalone terminal client binary (`ulwai-cli`) and `ulwai_core` library do **not** link against or require Qt in any way.

---

## 5. Verbatim Third-Party License Notices

### A. MD4C License (MIT)
```text
Copyright (c) 2016-2026 Martin Mitáš

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### B. nlohmann/json License (MIT)
```text
Copyright (c) 2013-2023 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### C. libcurl License (MIT/X-style)
```text
COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1996 - 2024, Daniel Stenberg, <daniel@haxx.se>, and many contributors.

All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization of the copyright holder.
```

### D. Qt 6 (LGPLv3 Compliance Statement)
```text
Qt is licensed under the GNU Lesser General Public License version 3 (LGPLv3).
ULWAI complies with LGPLv3 by linking dynamically against Qt libraries (or system-provided
shared objects). Users are free to replace or upgrade Qt libraries independently.
Source code for Qt is available at https://code.qt.io/.
```
