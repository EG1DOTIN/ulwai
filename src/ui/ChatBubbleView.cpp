/**
 * @file ChatBubbleView.cpp
 * @brief High-Performance Rich-Text Chat Bubble Renderer & Code Card Interactor.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Specialized `QTextBrowser` that renders multi-turn conversation messages as styled
 *   chat bubbles resembling modern ChatGPT / LibreChat desktop interfaces.
 *   Provides interactive code cards with syntax styling and one-click copy buttons.
 *
 * KEY RESPONSIBILITIES & FEATURES:
 *   1. Lightweight HTML/CSS Bubble Rendering:
 *      - User prompts are right-aligned / distinctively styled in dark violet cards.
 *      - Assistant responses are rendered with clean typography, markdown headers, and code fences.
 *   2. Code Card Interaction (`onAnchorClicked`):
 *      - Intercepts clicks on internal links formatted as `copy://<index>`.
 *      - Copies code blocks directly to clipboard via `QClipboard` and displays feedback tooltip.
 *      - Dispatches external web links (`http://`, `https://`) safely to the user's default browser.
 *   3. Real-Time Token Streaming (`appendStreamingToken`):
 *      - Minimizes DOM reflows during active AI generation by incrementally appending tokens
 *        directly to the active assistant bubble block.
 *   4. Interactive Font Scaling:
 *      - Supports `Ctrl + MouseWheel` zoom, dynamically scaling font sizes between 9pt and 32pt.
 */

#include "ulwai/ui/ChatBubbleView.hpp"
#include "ulwai/ui/UiStyles.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/Platform.hpp"

#if defined(__linux__)
#include <malloc.h>
#endif

#include <QScrollBar>
#include <QDesktopServices>
#include <QApplication>
#include <QClipboard>
#include <QToolTip>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <algorithm>

namespace ulwai::ui {

ChatBubbleView::ChatBubbleView(QWidget* parent)
    : QTextBrowser(parent)
{
    sortLatestFirst_ = false; // Chronological order like ChatGPT
    setObjectName("chatBrowser");
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setFocusPolicy(Qt::ClickFocus);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    document()->setUndoRedoEnabled(false);
    document()->setDefaultStyleSheet(QString::fromStdString(getChatDocumentCss(fontSize_)));
    connect(this, &QTextBrowser::anchorClicked, this, &ChatBubbleView::onAnchorClicked);
}

void ChatBubbleView::setFontSize(int size) {
    int clamped = std::clamp(size, 9, 32);
    if (fontSize_ == clamped) return;
    fontSize_ = clamped;
    document()->setDefaultStyleSheet(QString::fromStdString(getChatDocumentCss(fontSize_)));
    renderFromCache();
}

void ChatBubbleView::setSortLatestFirst(bool latestFirst) {
    if (sortLatestFirst_ == latestFirst) return;
    sortLatestFirst_ = latestFirst;
    renderFromCache();
}

void ChatBubbleView::zoomIn(int delta) {
    setFontSize(fontSize_ + delta);
}

void ChatBubbleView::zoomOut(int delta) {
    setFontSize(fontSize_ - delta);
}

void ChatBubbleView::resetZoom() {
    setFontSize(13);
}

void ChatBubbleView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }
    QTextBrowser::wheelEvent(event);
}

void ChatBubbleView::renderSession(const ChatSession& session) {
    activeSessionId_ = session.id;
    cachedMessages_ = session.messages;
    renderFromCache();
}

QString ChatBubbleView::formatMessageHtml(const ChatMessage& msg) {
    QString html;
    size_t codeOffset = currentCodeBlocks_.size();
    if (msg.role == Role::User) {
        html.append("<div class='message-row user-row'>");
        html.append("<div class='avatar-label avatar-user'><span class='avatar-icon'>👤</span> You <span class='time-label'>" +
                    QString::fromStdString(msg.timestamp) + "</span></div>");
        html.append("<div class='user-text'>" + QString::fromStdString(utils::markdownToHtml(msg.content, codeOffset, &currentCodeBlocks_)) + "</div>");
        html.append("</div>");
    } else if (msg.role == Role::Assistant) {
        html.append("<div class='message-row'>");
        html.append("<div class='assistant-bubble'>");
        html.append("<div class='avatar-label avatar-assistant'><span class='avatar-icon'>✦</span> ULWAI AI <span class='time-label'>" +
                    QString::fromStdString(msg.timestamp) + "</span></div>");

        if (msg.is_shell_command) {
            std::string cmd = utils::stripShellMarkdown(msg.content);
            html.append("<table class='shell-box' width='100%' cellpadding='0' cellspacing='0'>");
            html.append("<tr><td class='shell-cell'>");
            html.append("<div class='shell-title'>⚡ Generated Shell Command</div>");
            html.append("<div class='shell-cmd'>" + QString::fromStdString(utils::escapeHtml(cmd)) + "</div>");
            html.append("<div style='margin-top: 8px;'>");
            html.append("<a href='shell-exec://" + QString::fromStdString(utils::urlEncode(cmd)) +
                        "' class='copy-btn-link' style='color: #10a37f; margin-right: 12px;'>[▶ Execute]</a>");
            html.append("<a href='shell-copy://" + QString::fromStdString(utils::urlEncode(cmd)) +
                        "' class='copy-btn-link' style='color: #38bdf8;'>[📋 Copy Command]</a>");
            html.append("</div>");

            if (!msg.shell_command_output.empty()) {
                html.append("<div class='shell-output'><pre>" +
                            QString::fromStdString(utils::escapeHtml(msg.shell_command_output)) +
                            "</pre></div>");
            }
            html.append("</td></tr></table>");
        } else {
            html.append("<div>" + QString::fromStdString(utils::markdownToHtml(msg.content, codeOffset, &currentCodeBlocks_, msg.is_code_only)) + "</div>");
        }

        html.append("</div></div>");
    }
    return html;
}

void ChatBubbleView::renderFromCache() {
    currentCodeBlocks_.clear();

    QString html;
    html.append("<html><head></head><body>");

    if (cachedMessages_.empty()) {
        html.append("<div style='text-align: center; margin-top: 80px; color: #71717a;'>");
        html.append("<h2 style='color: #d4d4d8;'>How can I help you today?</h2>");
        html.append("<p style='color: #a1a1aa;'>ULWAI is an ultra-lightweight AI agent working under 100MB RAM.</p>");
        html.append("<p style='font-size: 11px; color: #52525b; margin-top: 12px;'>Tip: Use Ctrl + Mouse Scroll to zoom in/out.</p>");
        html.append("</div>");
        html.append("</body></html>");
        setHtml(html);
        return;
    }

    if (sortLatestFirst_) {
        for (auto it = cachedMessages_.rbegin(); it != cachedMessages_.rend(); ++it) {
            html.append(formatMessageHtml(*it));
        }
    } else {
        // Chronological order (oldest at top, latest at bottom, like ChatGPT)
        for (size_t i = 0; i < cachedMessages_.size(); ++i) {
            if (i + 1 == cachedMessages_.size()) {
                html.append("<a name='latest_msg'></a>");
            }
            html.append(formatMessageHtml(cachedMessages_[i]));
        }
    }

    html.append("</body></html>");
    setHtml(html);

    if (sortLatestFirst_) {
        verticalScrollBar()->setValue(0);
    } else {
        if (!cachedMessages_.empty()) {
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
            QTimer::singleShot(0, this, [this]() {
                verticalScrollBar()->setValue(verticalScrollBar()->maximum());
            });
        }
    }

#if defined(__linux__)
    malloc_trim(0);
#endif
}

void ChatBubbleView::appendMessage(const ChatMessage& msg) {
    if (cachedMessages_.empty() || isStreaming_) {
        cachedMessages_.push_back(msg);
        isStreaming_ = false;
        renderFromCache();
        return;
    }

    cachedMessages_.push_back(msg);
    QString msgHtml = formatMessageHtml(msg);

    QTextCursor cursor = textCursor();
    if (sortLatestFirst_) {
        cursor.movePosition(QTextCursor::Start);
        cursor.insertHtml(msgHtml);
        verticalScrollBar()->setValue(0);
    } else {
        cursor.movePosition(QTextCursor::End);
        cursor.insertHtml(msgHtml);
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        QTimer::singleShot(0, this, [this]() {
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        });
    }

#if defined(__linux__)
    malloc_trim(0);
#endif
}

void ChatBubbleView::startStreamingMessage() {
    isStreaming_ = true;
    streamingBuffer_.clear();

    if (sortLatestFirst_) {
        QString current = toHtml();
        int bodyPos = current.indexOf("<body");
        if (bodyPos != -1) {
            int closeTag = current.indexOf(">", bodyPos);
            if (closeTag != -1) {
                current.insert(closeTag + 1,
                    "<div class='message-row'><div class='assistant-bubble'>"
                    "<div class='avatar-label avatar-assistant'><span class='avatar-icon'>✦</span> ULWAI AI (generating...)</div>"
                    "<div id='stream-content'></div></div></div>");
                setHtml(current);
            }
        }
        streamCursor_ = textCursor();
        streamCursor_.movePosition(QTextCursor::Start);
        streamCursor_.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 2);
        verticalScrollBar()->setValue(0);
    } else {
        QString current = toHtml();
        if (current.contains("</body>")) {
            current.replace("</body>",
                "<div class='message-row'><div class='assistant-bubble'>"
                "<div class='avatar-label avatar-assistant'><span class='avatar-icon'>✦</span> ULWAI AI (generating...)</div>"
                "<div id='stream-content'></div></div></div></body>");
            setHtml(current);
        }
        streamCursor_ = textCursor();
        streamCursor_.movePosition(QTextCursor::End);
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
}

void ChatBubbleView::appendStreamingToken(const QString& token) {
    if (!isStreaming_) return;
    streamingBuffer_.append(token);

    streamCursor_.insertText(token);

    if (sortLatestFirst_) {
        verticalScrollBar()->setValue(0);
    } else {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
}

void ChatBubbleView::finishStreaming(const std::string& fullContent) {
    isStreaming_ = false;
    (void)fullContent;
    renderFromCache();
}

void ChatBubbleView::onAnchorClicked(const QUrl& link) {
    // Preserve vertical scroll position strictly so clicking links or copy code cards never jumps or scrolls
    int scrollPos = verticalScrollBar()->value();
    clearFocus();

    if (textCursor().hasSelection()) {
        QTextCursor c = textCursor();
        c.clearSelection();
        setTextCursor(c);
        verticalScrollBar()->setValue(scrollPos);
    }

    QString scheme = link.scheme();
    if (scheme == "http" || scheme == "https") {
        QDesktopServices::openUrl(link);
    } else if (scheme == "shell-exec") {
        emit shellCommandConfirmed(link.toString().mid(13)); // strip 'shell-exec://'
    } else if (scheme == "shell-copy") {
        QString raw = link.toString().mid(13); // strip 'shell-copy://'
        QString cmd = QUrl::fromPercentEncoding(raw.toUtf8());
        auto* cb = QApplication::clipboard();
        if (cb) {
            cb->setText(cmd, QClipboard::Clipboard);
            cb->setText(cmd, QClipboard::Selection);
        }
        QToolTip::showText(QCursor::pos(), "✔ Copied shell command to clipboard!", this, QRect(), 2000);
    } else if (scheme == "copy" || scheme == "copy-code") {
        bool ok = false;
        size_t idx = utils::parseCopyBlockIndex(link.toString().toStdString(), ok);
        if (ok && idx < currentCodeBlocks_.size()) {
            QString code = QString::fromStdString(currentCodeBlocks_[idx].code);
            auto* cb = QApplication::clipboard();
            if (cb) {
                cb->setText(code, QClipboard::Clipboard);
                cb->setText(code, QClipboard::Selection);
            }
            QToolTip::showText(QCursor::pos(), "✔ Copied code to clipboard!", this, QRect(), 2500);
            emit codeCopied(code);
        }
    }

    verticalScrollBar()->setValue(scrollPos);
}

} // namespace ulwai::ui
