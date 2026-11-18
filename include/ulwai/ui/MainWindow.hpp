/**
 * @file MainWindow.hpp
 * @brief Primary Desktop GUI Application Window & Asynchronous Worker Thread Interface.
 */

#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QThread>
#include <atomic>
#include "ulwai/Config.hpp"
#include "ulwai/ConversationManager.hpp"
#include "ulwai/ui/SessionSidebar.hpp"
#include "ulwai/ui/ChatBubbleView.hpp"

namespace ulwai::ui {

class PromptInputEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit PromptInputEdit(QWidget* parent = nullptr);
signals:
    void sendRequested();
protected:
    void keyPressEvent(QKeyEvent* event) override;
};

class AiWorker : public QThread {
    Q_OBJECT
public:
    AiWorker(std::shared_ptr<IApiClient> client,
             std::vector<ChatMessage> messages,
             std::string systemPrompt,
             QObject* parent = nullptr);

    void cancel();

signals:
    void tokenReceived(const QString& token);
    void finished(const QString& fullText, const QString& error);

protected:
    void run() override;

private:
    std::shared_ptr<IApiClient> client_;
    std::vector<ChatMessage> messages_;
    std::string systemPrompt_;
    std::atomic<bool> cancelled_{false};
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Config& config, ConversationManager& conversation, QWidget* parent = nullptr);
    ~MainWindow() override;

    void showUpdateNotification(const std::string& latestVer, const std::string& releaseUrl);
    void checkUpdatesAsync(bool silentIfCurrent = true);

    void applyZoom(int newSize, bool refreshFullStyle = false);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSendClicked();
    void onStopClicked();
    void onAttachFileClicked();
    void onRemoveAttachmentClicked();
    void onEngineChanged(int index);
    void onModelChanged(int index);
    void onSessionSelected(const QString& sessionId);
    void onNewChatRequested();
    void onSettingsRequested();
    void onAboutRequested();
    void onExportRequested();
    void onShellCommandConfirmed(const QString& command);

    void onTokenReceived(const QString& token);
    void onWorkerFinished(const QString& fullText, const QString& error);

    /**
     * @brief Triggered when the top bar search query or filter toggles change.
     */
    void onSearchTriggered();

    void updateMemoryUsageLabel();

private:
    Config& config_;
    ConversationManager& conversation_;

    SessionSidebar* sidebar_{nullptr};
    ChatBubbleView* chatView_{nullptr};
    PromptInputEdit* promptInput_{nullptr};
    QPushButton* sendBtn_{nullptr};
    QPushButton* stopBtn_{nullptr};
    QComboBox* engineSelector_{nullptr};
    QComboBox* modelSelector_{nullptr};
    QLineEdit* searchInput_{nullptr};
    QPushButton* searchBtn_{nullptr};
    QPushButton* searchCaseBtn_{nullptr};
    QPushButton* searchWholeWordBtn_{nullptr};
    QPushButton* searchRegexBtn_{nullptr};
    QLabel* memoryLabel_{nullptr};
    QWidget* topBar_{nullptr};
    QHBoxLayout* topBarLayout_{nullptr};
    QPushButton* aboutBtn_{nullptr};
    QPushButton* updateBannerBtn_{nullptr};
    QLabel* attachmentLabel_{nullptr};
    QPushButton* removeAttachmentBtn_{nullptr};

    QRadioButton* modeChatRadio_{nullptr};
    QRadioButton* modeCodeRadio_{nullptr};
    QRadioButton* modeShellRadio_{nullptr};

    std::string attachedFilePath_;
    std::string attachedFileContent_;

    AiWorker* currentWorker_{nullptr};
    std::string accumulatedStreamText_;
    bool isGenerating_{false};
    int zoomDeltaAccumulator_{0};

    void setupUi();
    void refreshActiveSessionView();
    [[nodiscard]] AgentMode getSelectedMode() const noexcept;
};

} // namespace ulwai::ui
