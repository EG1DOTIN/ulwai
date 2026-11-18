/**
 * @file UiStyles.hpp
 * @brief Modern ChatGPT / LibreChat Inspired Dark Theme QSS & HTML/CSS Stylesheets.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Centralized styling repository for ULWAI's desktop graphical user interface.
 *   Provides dynamic, DPI-aware Qt Style Sheets (QSS) for desktop widgets and HTML/CSS
 *   document styling for the chat message pane (`ChatBubbleView`).
 *
 * KEY FUNCTIONS:
 *   - `getDarkAppStylesheet(int baseFontSize)`: Comprehensive QSS for MainWindow, SessionSidebar,
 *     PromptInputEdit, and SettingsDialog.
 *   - `getChatDocumentCss(int baseFontSize)`: Web-standard CSS for Markdown bubbles, syntax-highlighted
 *     code cards, and copy buttons.
 */

#pragma once

#include <string>
#include <sstream>
#include <algorithm>

namespace ulwai::ui {

inline std::string getDarkAppStylesheet(int baseFontSize = 13) {
    int font = std::clamp(baseFontSize, 9, 32);
    int inputFont = font + 1;
    int padV = std::clamp(font / 2, 4, 12);
    int padH = std::clamp(font - 2, 8, 18);
    int smallFont = std::max(9, font - 2);
    std::ostringstream ss;
    ss << R"(
        QMainWindow {
            background-color: #171717;
            color: #ececec;
        }
        QWidget {
            font-family: "Noto Sans Devanagari", "Noto Sans", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "DejaVu Sans", "Liberation Sans", "Helvetica Neue", Arial, sans-serif;
            font-size: )" << font << R"(px;
            color: #ececec;
        }
        QSplitter::handle {
            background-color: #2b2b2b;
            width: 1px;
        }
        /* Buttons global focus & selection fix */
        QPushButton {
            outline: none;
        }
        QPushButton:focus, QPushButton:pressed {
            outline: none;
        }
        /* Sidebar (Left Side) */
        #sidebarWidget {
            background-color: #121212;
            border-right: 1px solid #282828;
        }
        QPushButton#newChatBtn {
            background-color: #242424;
            color: #ececec;
            border: 1px solid #383838;
            border-radius: 8px;
            padding: )" << padV << "px " << padH << R"(px;
            font-weight: 600;
            font-size: )" << font << R"(px;
            text-align: left;
            outline: none;
        }
        QPushButton#newChatBtn:hover {
            background-color: #303030;
        }
        QPushButton#newChatBtn:focus {
            outline: none;
            border: 1px solid #4a4a4a;
        }
        QListWidget#sessionList {
            background-color: transparent;
            border: none;
            outline: none;
            padding: 4px;
            font-size: )" << font << R"(px;
        }
        QListWidget#sessionList:focus {
            outline: none;
        }
        QListWidget#sessionList::item {
            padding: )" << padV << "px " << padH << R"(px;
            border-radius: 6px;
            margin-bottom: 2px;
            color: #b4b4b4;
            outline: none;
            white-space: nowrap;
            font-size: )" << font << R"(px;
        }
        QListWidget#sessionList::item:hover {
            background-color: #222222;
            color: #ffffff;
        }
        QListWidget#sessionList::item:selected {
            background-color: #2c2c2c;
            color: #ffffff;
            font-weight: 500;
            outline: none;
        }
        /* Title Search Bar in Top Bar */
        QLineEdit#titleSearchInput {
            background-color: #1c1c1f;
            border: 1px solid #38383f;
            border-radius: 6px;
            padding: 4px 8px;
            color: #ececec;
            font-size: )" << font << R"(px;
            selection-background-color: #10a37f;
            selection-color: #ffffff;
        }
        QLineEdit#titleSearchInput:focus {
            border: 1px solid #10b981;
            background-color: #222227;
        }
        /* Chat View (Right Side) */
        QTextBrowser#chatBrowser {
            background-color: #171717;
            border: none;
            padding: 16px;
            selection-background-color: #264f78;
            selection-color: #ffffff;
            outline: none;
        }
        QTextBrowser#chatBrowser:focus {
            outline: none;
        }
        /* Input area */
        #inputContainer {
            background-color: #171717;
            border-top: 1px solid #282828;
            padding: 12px 16px;
        }
        QTextEdit#promptInput {
            background-color: #242424;
            border: 1px solid #383838;
            border-radius: 12px;
            padding: 10px 14px;
            color: #ffffff;
            font-size: )" << inputFont << R"(px;
            selection-background-color: #264f78;
            selection-color: #ffffff;
        }
        QTextEdit#promptInput:focus {
            border: 1px solid #555555;
            outline: none;
        }
        QPushButton#sendBtn {
            background-color: #10a37f;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 600;
            font-size: )" << font << R"(px;
            outline: none;
        }
        QPushButton#sendBtn:hover {
            background-color: #1a7f64;
        }
        QPushButton#sendBtn:disabled {
            background-color: #383838;
            color: #777777;
        }
        QPushButton#stopBtn {
            background-color: #ef4444;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 600;
            font-size: )" << font << R"(px;
            outline: none;
        }
        QPushButton#stopBtn:hover {
            background-color: #dc2626;
        }
        QPushButton#iconBtn {
            background-color: transparent;
            color: #b4b4b4;
            border: 1px solid #383838;
            border-radius: 6px;
            padding: 6px 10px;
            font-size: )" << font << R"(px;
            outline: none;
        }
        QPushButton#iconBtn:hover {
            background-color: #262626;
            color: #ffffff;
        }
        QComboBox {
            background-color: #262626;
            border: 1px solid #383838;
            border-radius: 6px;
            padding: 6px 12px;
            color: #ffffff;
            font-size: )" << font << R"(px;
            outline: none;
        }
        QComboBox:focus {
            outline: none;
            border: 1px solid #555555;
        }
        QComboBox QAbstractItemView {
            background-color: #202020;
            color: #ffffff;
            selection-background-color: #333333;
            border: 1px solid #383838;
            outline: none;
            font-size: )" << font << R"(px;
        }
        /* Mode Pills */
        QRadioButton {
            color: #b4b4b4;
            spacing: 6px;
            font-size: )" << font << R"(px;
            outline: none;
        }
        QRadioButton:focus {
            outline: none;
        }
        QRadioButton:checked {
            color: #10a37f;
            font-weight: bold;
        }
        QStatusBar {
            background-color: #121212;
            color: #888888;
            border-top: 1px solid #282828;
            font-size: )" << smallFont << R"(px;
        }
    )";
    return ss.str();
}

inline std::string getSettingsDialogStylesheet(int baseFontSize = 13) {
    int font = std::clamp(baseFontSize, 12, 24);
    int inputFont = font;
    int titleFont = font + 1;

    std::ostringstream ss;
    ss << R"(
        QDialog {
            background-color: #1a1a20;
            color: #f4f4f5;
            font-family: "Noto Sans Devanagari", "Noto Sans", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "DejaVu Sans", "Liberation Sans", "Helvetica Neue", Arial, sans-serif;
            font-size: )" << font << R"(px;
        }
        QFrame#settingsRootFrame {
            background-color: #1a1a20;
            border: 2px solid #5a5a6e;
            border-radius: 0px;
        }
        QWidget {
            color: #f4f4f5;
            font-family: "Noto Sans Devanagari", "Noto Sans", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "DejaVu Sans", "Liberation Sans", "Helvetica Neue", Arial, sans-serif;
            font-size: )" << font << R"(px;
        }
        /* Tab Widget Styling */
        QTabWidget::pane {
            border: 1px solid #383842;
            background-color: #141418;
            border-radius: 6px;
            top: -1px;
            padding: 8px;
        }
        QTabBar::tab {
            background-color: #242429;
            color: #a1a1aa;
            border: 1px solid #383842;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            padding: 9px 20px;
            min-height: 20px;
            margin-right: 4px;
            margin-top: 3px;
            font-weight: 500;
            font-size: )" << font << R"(px;
        }
        QTabBar::tab:selected {
            background-color: #141418;
            color: #ffffff;
            border: 1px solid #383842;
            border-bottom: 1px solid #141418;
            border-top: 2px solid #10b981;
            margin-top: 0px;
            font-weight: 600;
        }
        QTabBar::tab:hover:!selected {
            background-color: #2d2d34;
            color: #ffffff;
        }

        /* Group Box Styling */
        QGroupBox {
            background-color: #18181e;
            border: 1px solid #383844;
            border-radius: 8px;
            margin-top: 14px;
            padding-top: 16px;
            padding-bottom: 12px;
            padding-left: 14px;
            padding-right: 14px;
            font-weight: 600;
            font-size: )" << titleFont << R"(px;
            color: #f4f4f5;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 0 6px;
            background-color: #18181e;
            color: #f4f4f5;
        }
        QGroupBox#dangerGroup {
            background-color: #221314;
            border: 1px solid #991b1b;
            border-radius: 8px;
            margin-top: 14px;
            padding-top: 16px;
            padding-bottom: 12px;
            padding-left: 14px;
            padding-right: 14px;
            font-weight: 600;
            font-size: )" << titleFont << R"(px;
            color: #f87171;
        }
        QGroupBox#dangerGroup::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 0 6px;
            background-color: #221314;
            color: #ef4444;
            font-weight: 600;
        }

        /* Labels */
        QLabel {
            color: #e4e4e7;
            font-size: )" << font << R"(px;
        }
        QLabel#descLabel {
            color: #d1d5db;
            font-size: )" << font << R"(px;
            line-height: 1.4;
        }
        QLabel#dangerDescLabel {
            color: #fca5a5;
            font-size: )" << font << R"(px;
            font-weight: normal;
            line-height: 1.4;
        }

        /* Form Inputs */
        QLineEdit, QTextEdit, QSpinBox {
            background-color: #18181b;
            border: 1px solid #3f3f46;
            border-radius: 6px;
            padding: 7px 10px;
            color: #ffffff;
            font-size: )" << inputFont << R"(px;
            selection-background-color: #2563eb;
            selection-color: #ffffff;
        }
        QLineEdit:focus, QTextEdit:focus, QSpinBox:focus {
            border: 1px solid #10b981;
            outline: none;
        }

        /* Combo Boxes */
        QComboBox {
            background-color: #18181b;
            border: 1px solid #3f3f46;
            border-radius: 6px;
            padding: 7px 12px;
            color: #ffffff;
            font-size: )" << font << R"(px;
        }
        QComboBox:focus {
            border: 1px solid #10b981;
            outline: none;
        }
        QComboBox QAbstractItemView {
            background-color: #27272a;
            color: #ffffff;
            selection-background-color: #3f3f46;
            selection-color: #ffffff;
            border: 1px solid #4b5563;
            outline: none;
            padding: 4px;
            font-size: )" << font << R"(px;
        }

        /* General Buttons */
        QPushButton {
            background-color: #27272a;
            color: #ffffff;
            border: 1px solid #4b5563;
            border-radius: 6px;
            padding: 8px 16px;
            font-size: )" << font << R"(px;
            font-weight: 500;
            outline: none;
        }
        QPushButton:hover {
            background-color: #3f3f46;
            border-color: #6b7280;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #18181b;
        }

        /* Top Bar Search Button & Filter Buttons */
        QPushButton#searchBtn {
            background-color: #242427;
            color: #d4d4d8;
            border: 1px solid #3f3f46;
            border-radius: 6px;
            padding: 0px;
            margin: 0px;
            font-size: 13px;
            font-weight: normal;
            outline: none;
        }
        QPushButton#searchBtn:hover {
            background-color: #333338;
            border: 1px solid #10b981;
            color: #ffffff;
        }
        QPushButton#searchBtn:pressed {
            background-color: #10b981;
            color: #ffffff;
        }
        QPushButton#searchFilterBtn {
            background-color: #202023;
            color: #a1a1aa;
            border: 1px solid #38383e;
            border-radius: 6px;
            padding: 0px;
            margin: 0px;
            font-size: 11px;
            font-weight: 700;
            outline: none;
        }
        QPushButton#searchFilterBtn:hover {
            background-color: #2d2d33;
            color: #ffffff;
            border: 1px solid #52525b;
        }
        QPushButton#searchFilterBtn:checked {
            background-color: #064e3b;
            color: #34d399;
            border: 1px solid #10b981;
        }

        /* Action Buttons (Import / Export / Zoom) */
        QPushButton#actionBtn {
            background-color: #2d3139;
            color: #ffffff;
            border: 1px solid #525a68;
            border-radius: 6px;
            padding: 9px 16px;
            font-weight: 600;
            font-size: )" << font << R"(px;
        }
        QPushButton#actionBtn:hover {
            background-color: #3d434f;
            border-color: #717d91;
            color: #ffffff;
        }
        QPushButton#actionBtn:pressed {
            background-color: #20242b;
        }

        /* Primary Green Button (Save Settings) */
        QPushButton#primaryBtn {
            background-color: #10a37f;
            color: #ffffff;
            border: 1px solid #059669;
            border-radius: 6px;
            padding: 8px 20px;
            font-weight: 600;
            font-size: )" << font << R"(px;
        }
        QPushButton#primaryBtn:hover {
            background-color: #059669;
        }
        QPushButton#primaryBtn:pressed {
            background-color: #047857;
        }

        /* Danger Red Button */
        QPushButton#dangerBtn {
            background-color: #b91c1c;
            color: #ffffff;
            border: 1px solid #dc2626;
            border-radius: 6px;
            padding: 10px 18px;
            font-weight: 600;
            font-size: )" << font << R"(px;
        }
        QPushButton#dangerBtn:hover {
            background-color: #dc2626;
        }
        QPushButton#dangerBtn:pressed {
            background-color: #991b1b;
        }

        /* Checkbox & Slider */
        QCheckBox {
            color: #e4e4e7;
            spacing: 8px;
            font-size: )" << font << R"(px;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: #3f3f46;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: #10a37f;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #ffffff;
            width: 16px;
            margin-top: -5px;
            margin-bottom: -5px;
            border-radius: 8px;
        }

        /* Scroll Area & Scrollbar */
        QScrollArea, QScrollArea > QWidget > QWidget {
            background-color: transparent;
            border: none;
        }
        QScrollBar:horizontal {
            height: 0px;
        }
        QScrollBar:vertical {
            background-color: transparent;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background-color: #3f3f46;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #52525b;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )";
    return ss.str();
}

inline std::string getChatDocumentCss(int baseFontSize = 13) {
    int font = std::clamp(baseFontSize, 9, 32);
    int fontUser = font;
    int fontAssistant = font;
    int fontCode = font;
    int fontInline = std::max(9, font - 1);
    int fontAvatar = std::max(11, font);
    int fontTime = std::max(9, font - 3);
    int fontShellTitle = std::max(11, font);
    int fontShellCmd = font;
    int fontShellOutput = std::max(9, font - 1);

    std::ostringstream css;
    css << "body {\n"
        << "    font-family: 'Noto Sans Devanagari', 'Noto Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'DejaVu Sans', 'Liberation Sans', Helvetica, Arial, sans-serif;\n"
        << "    background-color: #171717;\n"
        << "    color: #ececec;\n"
        << "    margin: 0;\n"
        << "    padding: 14px;\n"
        << "    line-height: 1.6;\n"
        << "    font-size: " << font << "px;\n"
        << "}\n"
        << ".message-row {\n"
        << "    margin-bottom: 22px;\n"
        << "}\n"
        << ".user-row {\n"
        << "    padding: 4px 10px;\n"
        << "    margin-bottom: 18px;\n"
        << "}\n"
        << ".user-text {\n"
        << "    color: #f4f4f5;\n"
        << "    font-size: " << fontUser << "px;\n"
        << "    line-height: 1.6;\n"
        << "}\n"
        << ".assistant-bubble {\n"
        << "    background-color: transparent;\n"
        << "    border: none;\n"
        << "    padding: 4px 10px;\n"
        << "    margin-right: 20px;\n"
        << "    margin-bottom: 8px;\n"
        << "    font-size: " << fontAssistant << "px;\n"
        << "    color: #ececec;\n"
        << "}\n"
        << ".avatar-label {\n"
        << "    font-weight: 700;\n"
        << "    font-size: " << fontAvatar << "px;\n"
        << "    margin-bottom: 8px;\n"
        << "    letter-spacing: 0.2px;\n"
        << "}\n"
        << ".avatar-user {\n"
        << "    color: #60a5fa;\n"
        << "}\n"
        << ".avatar-assistant {\n"
        << "    color: #10a37f;\n"
        << "}\n"
        << ".avatar-icon {\n"
        << "    font-size: 1.1em;\n"
        << "    margin-right: 4px;\n"
        << "}\n"
        << ".time-label {\n"
        << "    font-size: " << fontTime << "px;\n"
        << "    font-weight: normal;\n"
        << "    color: #71717a;\n"
        << "    margin-left: 8px;\n"
        << "}\n"
        << ".code-card {\n"
        << "    background-color: #0d0d0d;\n"
        << "    border: 1px solid #333333;\n"
        << "    border-radius: 8px;\n"
        << "    margin: 14px 0;\n"
        << "}\n"
        << ".code-header {\n"
        << "    background-color: #262626;\n"
        << "    color: #d4d4d8;\n"
        << "    font-size: " << fontTime << "px;\n"
        << "    font-family: 'Noto Sans Devanagari', 'Noto Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'DejaVu Sans', monospace;\n"
        << "    font-weight: 600;\n"
        << "    padding: 6px 12px;\n"
        << "    border-bottom: 1px solid #333333;\n"
        << "    border-top-left-radius: 8px;\n"
        << "    border-top-right-radius: 8px;\n"
        << "}\n"
        << ".code-lang {\n"
        << "    color: #a1a1aa;\n"
        << "    text-transform: uppercase;\n"
        << "    font-weight: 700;\n"
        << "    letter-spacing: 0.5px;\n"
        << "}\n"
        << ".copy-btn-link {\n"
        << "    color: #e4e4e7;\n"
        << "    background-color: #383838;\n"
        << "    text-decoration: none;\n"
        << "    font-size: " << fontTime << "px;\n"
        << "    font-weight: 600;\n"
        << "    padding: 3px 8px;\n"
        << "    border-radius: 4px;\n"
        << "    outline: none;\n"
        << "}\n"
        << ".copy-btn-link:hover {\n"
        << "    background-color: #4a4a4a;\n"
        << "    color: #ffffff;\n"
        << "}\n"
        << ".code-body-cell {\n"
        << "    background-color: #0d0d0d;\n"
        << "    padding: 12px 16px;\n"
        << "}\n"
        << ".code-body {\n"
        << "    background-color: transparent;\n"
        << "    padding: 0;\n"
        << "    margin: 0;\n"
        << "    font-family: 'Fira Code', 'JetBrains Mono', Consolas, 'Courier New', monospace;\n"
        << "    font-size: " << fontCode << "px;\n"
        << "    color: #38bdf8;\n"
        << "    white-space: pre-wrap;\n"
        << "    line-height: 1.5;\n"
        << "}\n"
        << ".inline-code {\n"
        << "    background-color: #27272a;\n"
        << "    color: #f43f5e;\n"
        << "    padding: 2px 6px;\n"
        << "    border-radius: 4px;\n"
        << "    font-family: monospace;\n"
        << "    font-size: " << fontInline << "px;\n"
        << "}\n"
        << ".md-p {\n"
        << "    margin: 0 0 10px 0;\n"
        << "    line-height: 1.6;\n"
        << "}\n"
        << ".md-h1 {\n"
        << "    font-size: 1.4em;\n"
        << "    font-weight: 700;\n"
        << "    color: #ffffff;\n"
        << "    margin: 16px 0 8px 0;\n"
        << "    border-bottom: 1px solid #333;\n"
        << "    padding-bottom: 4px;\n"
        << "}\n"
        << ".md-h2 {\n"
        << "    font-size: 1.25em;\n"
        << "    font-weight: 600;\n"
        << "    color: #f4f4f5;\n"
        << "    margin: 14px 0 6px 0;\n"
        << "}\n"
        << ".md-h3 {\n"
        << "    font-size: 1.1em;\n"
        << "    font-weight: 600;\n"
        << "    color: #e4e4e7;\n"
        << "    margin: 12px 0 4px 0;\n"
        << "}\n"
        << ".md-h4 {\n"
        << "    font-size: 1.0em;\n"
        << "    font-weight: 600;\n"
        << "    color: #d4d4d8;\n"
        << "    margin: 10px 0 4px 0;\n"
        << "}\n"
        << ".md-ul, .md-ol {\n"
        << "    margin: 6px 0 10px 24px;\n"
        << "    padding-left: 0;\n"
        << "}\n"
        << ".md-li {\n"
        << "    margin-bottom: 4px;\n"
        << "    line-height: 1.5;\n"
        << "}\n"
        << ".md-quote {\n"
        << "    border-left: 3px solid #10a37f;\n"
        << "    background-color: #1a2520;\n"
        << "    margin: 10px 0;\n"
        << "    padding: 8px 14px;\n"
        << "    border-radius: 0 6px 6px 0;\n"
        << "    color: #d1fae5;\n"
        << "}\n"
        << ".md-hr {\n"
        << "    border: 0;\n"
        << "    border-top: 1px solid #333;\n"
        << "    margin: 16px 0;\n"
        << "}\n"
        << ".md-table {\n"
        << "    border-collapse: collapse;\n"
        << "    width: 100%;\n"
        << "    margin: 12px 0;\n"
        << "}\n"
        << ".md-table th {\n"
        << "    background-color: #27272a;\n"
        << "    color: #ffffff;\n"
        << "    font-weight: 600;\n"
        << "    border: 1px solid #3f3f46;\n"
        << "    padding: 8px 12px;\n"
        << "    text-align: left;\n"
        << "}\n"
        << ".md-table td {\n"
        << "    border: 1px solid #3f3f46;\n"
        << "    padding: 7px 12px;\n"
        << "    color: #e4e4e7;\n"
        << "}\n"
        << ".md-table tr:nth-child(even) {\n"
        << "    background-color: #18181b;\n"
        << "}\n"
        << ".shell-box {\n"
        << "    background-color: #141414;\n"
        << "    border: 1px solid #f59e0b;\n"
        << "    border-radius: 8px;\n"
        << "    margin: 12px 0;\n"
        << "}\n"
        << ".shell-cell {\n"
        << "    background-color: #141414;\n"
        << "    padding: 12px 14px;\n"
        << "}\n"
        << ".shell-title {\n"
        << "    color: #f59e0b;\n"
        << "    font-weight: bold;\n"
        << "    font-size: " << fontShellTitle << "px;\n"
        << "    margin-bottom: 6px;\n"
        << "}\n"
        << ".shell-cmd {\n"
        << "    font-family: monospace;\n"
        << "    color: #fef08a;\n"
        << "    background-color: #202020;\n"
        << "    padding: 8px 12px;\n"
        << "    border-radius: 4px;\n"
        << "    font-size: " << fontShellCmd << "px;\n"
        << "}\n"
        << ".shell-output {\n"
        << "    background-color: #000000;\n"
        << "    color: #a8a29e;\n"
        << "    font-family: monospace;\n"
        << "    padding: 8px 12px;\n"
        << "    margin-top: 8px;\n"
        << "    border-radius: 4px;\n"
        << "    font-size: " << fontShellOutput << "px;\n"
        << "}\n"
        << "a {\n"
        << "    text-decoration: none;\n"
        << "    outline: none;\n"
        << "}\n"
        << "a:focus, a:active {\n"
        << "    outline: none;\n"
        << "}\n";
    return css.str();
}

} // namespace ulwai::ui
