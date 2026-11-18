/**
 * @file SessionSidebar.hpp
 * @brief Conversation History Navigation & Session Management Sidebar Interface.
 */

#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include "ulwai/ConversationManager.hpp"

namespace ulwai::ui {

class SessionSidebar : public QWidget {
    Q_OBJECT

public:
    explicit SessionSidebar(ConversationManager& conversation, QWidget* parent = nullptr);

    void refreshSessions();
    void selectSession(const std::string& sessionId);
    void updateFontSize(int size);

    /**
     * @brief Filters sidebar sessions in-place by title using configurable match options.
     * @param query Search query substring or pattern.
     * @param options Case sensitivity, whole word, and regex options.
     */
    void filterSessionsByTitle(const QString& query, const TitleSearchOptions& options = {});

    /**
     * @brief Clears active search filter and unhides all conversation sessions.
     */
    void clearFilter();

    /**
     * @brief Total number of sessions stored and loaded in sidebar.
     */
    [[nodiscard]] int totalSessionCount() const noexcept;

    /**
     * @brief Number of sessions currently visible matching the active search query.
     */
    [[nodiscard]] int visibleSessionCount() const noexcept;

signals:
    void sessionSelected(const QString& sessionId);
    void newChatRequested();
    void settingsRequested();
    void exportRequested();

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onNewChatClicked();
    void showContextMenu(const QPoint& pos);

private:
    ConversationManager& conversation_;
    QListWidget* sessionList_{nullptr};
    QPushButton* newChatBtn_{nullptr};
    QPushButton* settingsBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QString currentFilterQuery_;
    TitleSearchOptions currentFilterOptions_{};

    void setupUi();
};

} // namespace ulwai::ui
