/**
 * @file ChatBubbleView.hpp
 * @brief High-Performance Rich-Text Chat Bubble Renderer & Code Card Component Interface.
 */

#pragma once

#include <QTextBrowser>
#include <QString>
#include <QTextCursor>
#include <vector>
#include "ulwai/Types.hpp"

namespace ulwai::ui {

class ChatBubbleView : public QTextBrowser {
    Q_OBJECT

public:
    explicit ChatBubbleView(QWidget* parent = nullptr);

    /**
     * @brief Performs a full re-render of all messages in the specified session.
     * @param session Active chat session to render.
     */
    void renderSession(const ChatSession& session);

    /**
     * @brief Incrementally appends a single message bubble in O(1) time without full document re-render.
     * @param msg The newly added user or assistant message.
     */
    void appendMessage(const ChatMessage& msg);
    void startStreamingMessage();
    void appendStreamingToken(const QString& token);
    void finishStreaming(const std::string& fullContent);

    void setFontSize(int size);
    [[nodiscard]] int getFontSize() const noexcept { return fontSize_; }

    void setSortLatestFirst(bool latestFirst);
    [[nodiscard]] bool isSortLatestFirst() const noexcept { return sortLatestFirst_; }

public slots:
    void zoomIn(int delta = 1);
    void zoomOut(int delta = 1);
    void resetZoom();

signals:
    void shellCommandConfirmed(const QString& command);
    void fontSizeChanged(int newSize);
    void codeCopied(const QString& snippet);

protected:
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onAnchorClicked(const QUrl& link);

private:
    int fontSize_{13};
    bool sortLatestFirst_{false}; // Default: Chronological (User prompt first, then AI response, like ChatGPT/Gemini)
    std::vector<CodeBlock> currentCodeBlocks_;
    bool isStreaming_{false};
    QString streamingBuffer_;
    std::string activeSessionId_;
    std::vector<ChatMessage> cachedMessages_;
    QTextCursor streamCursor_;

    void renderFromCache();
    QString formatMessageHtml(const ChatMessage& msg);
    void appendHtmlSafe(const QString& html);
};

} // namespace ulwai::ui
