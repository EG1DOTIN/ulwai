/**
 * @file MainWindow.cpp
 * @brief Primary Desktop GUI Application Window, Layouts & Async Worker Thread.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements the core desktop user interface for ULWAI (`ulwai` and `ulw-ai-agent`).
 *   Constructs the full visual layout including the conversation sidebar, chat view,
 *   engine/model pickers, prompt input box, and real-time status bar.
 *
 * KEY COMPONENTS & ARCHITECTURE:
 *   1. `AiWorker` (Asynchronous Threading):
 *      - Dedicated background `QThread` that executes `IApiClient::chatStream`.
 *      - Emits Qt signals (`tokenAvailable`, `finished`, `errorOccurred`) marshalled safely
 *        across thread boundaries onto the Qt event loop, preventing UI hangs during network I/O.
 *   2. `PromptInputEdit`:
 *      - Custom `QTextEdit` that intercepts `Enter` to submit prompts, and `Shift+Enter` / `Ctrl+Enter`
 *        for multiline authoring.
 *   3. Layout Composition:
 *      - Left: `SessionSidebar` collapsible tree displaying past conversations.
 *      - Top: Model selector dropdown, engine switcher, and sidebar toggle.
 *      - Center: `ChatBubbleView` rendering formatted HTML bubbles and code cards.
 *      - Bottom: Multiline prompt input, Send/Stop controls, and File Attachment button.
 *   4. Shell Execution Dialog:
 *      - When AI generates shell commands in Shell mode, presents a safety confirmation dialog
 *        showing the exact command before executing via `platform::executeCommand`.
 *   5. Memory Monitoring & Footprint Guarantee:
 *      - Real-time memory monitor in status bar reads process RSS to verify memory stays strictly <100MB.
 */

#include "ulwai/ui/MainWindow.hpp"
#include "ulwai/ui/UiStyles.hpp"
#include "ulwai/ui/SettingsDialog.hpp"
#include "ulwai/PollinationsClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/Platform.hpp"
#include "ulwai/Version.hpp"

#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QPointer>
#include <fstream>
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#include <malloc.h>
#endif

namespace ulwai::ui {

// ─── PromptInputEdit ──────────────────────────────────────────────────────────
PromptInputEdit::PromptInputEdit(QWidget* parent) : QTextEdit(parent) {
    setObjectName("promptInput");
    setPlaceholderText("Message ULWAI... (Press Enter to send, Shift+Enter for new line)");
    setFixedHeight(70);
}

void PromptInputEdit::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & Qt::ShiftModifier || event->modifiers() & Qt::ControlModifier) {
            QTextEdit::keyPressEvent(event);
        } else {
            emit sendRequested();
            event->accept();
            return;
        }
    } else {
        QTextEdit::keyPressEvent(event);
    }
}

// ─── AiWorker ─────────────────────────────────────────────────────────────────
AiWorker::AiWorker(std::shared_ptr<IApiClient> client,
                   std::vector<ChatMessage> messages,
                   std::string systemPrompt,
                   QObject* parent)
    : QThread(parent),
      client_(std::move(client)),
      messages_(std::move(messages)),
      systemPrompt_(std::move(systemPrompt)) {}

void AiWorker::cancel() {
    cancelled_ = true;
}

void AiWorker::run() {
    if (!client_) {
        emit finished("", "Client not initialized");
        return;
    }

    auto onToken = [this](std::string_view token) {
        if (!cancelled_) {
            emit tokenReceived(QString::fromUtf8(token.data(), static_cast<int>(token.size())));
        }
    };

    try {
        std::string full = client_->chat(messages_, systemPrompt_, onToken);
        if (!cancelled_) {
            emit finished(QString::fromStdString(full), "");
        }
    } catch (const std::exception& e) {
        emit finished("", QString::fromStdString(e.what()));
    }
}

// ─── MainWindow ───────────────────────────────────────────────────────────────
MainWindow::MainWindow(Config& config, ConversationManager& conversation, QWidget* parent)
    : QMainWindow(parent), config_(config), conversation_(conversation)
{
    QString ver = QString::fromUtf8(ulwai::getVersion().data(), static_cast<int>(ulwai::getVersion().size()));
    setWindowTitle(QString::fromStdString(config_.app_name) + " v" + ver + " – Ultra-Lightweight AI Agent");
    if (std::filesystem::exists("resources/icons/ulwai.ico")) {
        setWindowIcon(QIcon("resources/icons/ulwai.ico"));
    } else if (std::filesystem::exists("resources/icons/ulwai-icon-128.png")) {
        setWindowIcon(QIcon("resources/icons/ulwai-icon-128.png"));
    }
    resize(1000, 700);

    setupUi();
    chatView_->setSortLatestFirst(config_.chat_sort_latest_first);
    applyZoom(config_.font_size, true);
    refreshActiveSessionView();

    qApp->installEventFilter(this);

    // Periodic memory usage updater
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateMemoryUsageLabel);
    timer->start(2000);
    updateMemoryUsageLabel();

    // Check for updates asynchronously in background after window initializes
    QTimer::singleShot(2500, this, [this]() {
        checkUpdatesAsync(true);
    });
}

MainWindow::~MainWindow() {
    if (currentWorker_ && currentWorker_->isRunning()) {
        currentWorker_->cancel();
        currentWorker_->wait(1000);
    }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            zoomDeltaAccumulator_ += wheelEvent->angleDelta().y();
            if (std::abs(zoomDeltaAccumulator_) >= 120) {
                int steps = zoomDeltaAccumulator_ / 120;
                zoomDeltaAccumulator_ %= 120;
                applyZoom(config_.font_size + steps, false);
            }
            wheelEvent->accept();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::applyZoom(int newSize, bool refreshFullStyle) {
    int clamped = std::clamp(newSize, 9, 32);
    if (!refreshFullStyle && clamped == config_.font_size) {
        return;
    }
    config_.font_size = clamped;

    if (refreshFullStyle) {
        setStyleSheet(QString::fromStdString(getDarkAppStylesheet(clamped)));
    }

    if (sidebar_) {
        sidebar_->updateFontSize(clamped);
    }
    if (chatView_) {
        chatView_->setFontSize(clamped);
    }
    if (promptInput_) {
        QFont inputFont = promptInput_->font();
        inputFont.setPixelSize(clamped + 1);
        promptInput_->setFont(inputFont);
    }

    statusBar()->showMessage(QString("Zoom: %1px (Ctrl+Scroll or Ctrl++/Ctrl- to adjust)").arg(clamped), 1500);

#if defined(__linux__)
    malloc_trim(0);
#endif
}

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Top Header Bar
    topBar_ = new QWidget(this);
    topBar_->setStyleSheet("background-color: #171717; border-bottom: 1px solid #303030; padding: 4px 16px;");
    topBarLayout_ = new QHBoxLayout(topBar_);
    topBarLayout_->setContentsMargins(12, 4, 12, 4);

    auto* logoLabel = new QLabel(topBar_);
    logoLabel->setObjectName("topBarLogo");
    bool logoLoaded = false;
    if (std::filesystem::exists("resources/icons/ulwai-logo.svg")) {
        QPixmap logoPix("resources/icons/ulwai-logo.svg");
        if (!logoPix.isNull()) {
            logoLabel->setPixmap(logoPix.scaledToHeight(44, Qt::SmoothTransformation));
            logoLoaded = true;
        }
    }
    if (!logoLoaded && std::filesystem::exists("resources/icons/ulwai-logo.png")) {
        QPixmap logoPix("resources/icons/ulwai-logo.png");
        if (!logoPix.isNull()) {
            logoLabel->setPixmap(logoPix.scaledToHeight(44, Qt::SmoothTransformation));
            logoLoaded = true;
        }
    }
    if (!logoLoaded) {
        logoLabel->setText("<b>ULWAI</b> <span style='color:#10a37f;'>AI AGENT</span>");
        logoLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    }
    topBarLayout_->addWidget(logoLabel);

    topBarLayout_->addSpacing(20);

    topBarLayout_->addWidget(new QLabel("Engine:", topBar_));
    engineSelector_ = new QComboBox(topBar_);
    engineSelector_->setFocusPolicy(Qt::NoFocus);
    engineSelector_->addItem("Pollinations (Free AI)", "pollinations");
    engineSelector_->addItem("DuckDuckGo (Facts)", "duckduckgo");
    engineSelector_->addItem("Ollama (Local)", "ollama");
    engineSelector_->addItem("OpenAI (Cloud)", "openai");
    engineSelector_->addItem("OpenRouter", "openrouter");
    engineSelector_->addItem("xAI Grok", "grok");
    engineSelector_->addItem("Hugging Face", "huggingface");

    int idx = engineSelector_->findData(QString::fromStdString(config_.default_engine));
    if (idx >= 0) engineSelector_->setCurrentIndex(idx);
    connect(engineSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onEngineChanged);
    topBarLayout_->addWidget(engineSelector_);

    topBarLayout_->addSpacing(10);
    topBarLayout_->addWidget(new QLabel("Model:", topBar_));
    modelSelector_ = new QComboBox(topBar_);
    modelSelector_->setFocusPolicy(Qt::NoFocus);
    modelSelector_->setMinimumWidth(160);
    connect(modelSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModelChanged);
    topBarLayout_->addWidget(modelSelector_);

    // Search Box & Filters (just after Model input on top right)
    topBarLayout_->addSpacing(16);

    auto* searchContainer = new QWidget(topBar_);
    auto* searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(4);

    searchInput_ = new QLineEdit(searchContainer);
    searchInput_->setObjectName("titleSearchInput");
    searchInput_->setPlaceholderText("Search titles...");
    searchInput_->setClearButtonEnabled(true);
    searchInput_->setMinimumWidth(140);
    searchInput_->setMaximumWidth(200);
    connect(searchInput_, &QLineEdit::textChanged, this, &MainWindow::onSearchTriggered);
    connect(searchInput_, &QLineEdit::returnPressed, this, &MainWindow::onSearchTriggered);
    searchLayout->addWidget(searchInput_);

    searchBtn_ = new QPushButton("🔍", searchContainer);
    searchBtn_->setObjectName("searchBtn");
    searchBtn_->setFixedSize(28, 28);
    searchBtn_->setToolTip("Search chat titles (Enter)");
    searchBtn_->setFocusPolicy(Qt::NoFocus);
    searchBtn_->setStyleSheet("QPushButton { background-color: #242427; color: #d4d4d8; border: 1px solid #3f3f46; border-radius: 6px; padding: 0px; font-size: 13px; } QPushButton:hover { background-color: #333338; border: 1px solid #10b981; }");
    connect(searchBtn_, &QPushButton::clicked, this, &MainWindow::onSearchTriggered);
    searchLayout->addWidget(searchBtn_);

    const QString filterBtnStyle = "QPushButton { background-color: #202023; color: #a1a1aa; border: 1px solid #38383e; border-radius: 6px; padding: 0px; font-size: 11px; font-weight: bold; } QPushButton:hover { background-color: #2d2d33; color: #ffffff; border: 1px solid #52525b; } QPushButton:checked { background-color: #064e3b; color: #34d399; border: 1px solid #10b981; }";

    searchCaseBtn_ = new QPushButton("Aa", searchContainer);
    searchCaseBtn_->setObjectName("searchCaseBtn");
    searchCaseBtn_->setCheckable(true);
    searchCaseBtn_->setFixedSize(28, 28);
    searchCaseBtn_->setToolTip("Match Case (Case Sensitive)");
    searchCaseBtn_->setFocusPolicy(Qt::NoFocus);
    searchCaseBtn_->setStyleSheet(filterBtnStyle);
    connect(searchCaseBtn_, &QPushButton::toggled, this, &MainWindow::onSearchTriggered);
    searchLayout->addWidget(searchCaseBtn_);

    searchWholeWordBtn_ = new QPushButton("W", searchContainer);
    searchWholeWordBtn_->setObjectName("searchWholeWordBtn");
    searchWholeWordBtn_->setCheckable(true);
    searchWholeWordBtn_->setFixedSize(28, 28);
    searchWholeWordBtn_->setToolTip("Match Whole Word");
    searchWholeWordBtn_->setFocusPolicy(Qt::NoFocus);
    searchWholeWordBtn_->setStyleSheet(filterBtnStyle);
    connect(searchWholeWordBtn_, &QPushButton::toggled, this, &MainWindow::onSearchTriggered);
    searchLayout->addWidget(searchWholeWordBtn_);

    searchRegexBtn_ = new QPushButton(".*", searchContainer);
    searchRegexBtn_->setObjectName("searchRegexBtn");
    searchRegexBtn_->setCheckable(true);
    searchRegexBtn_->setFixedSize(28, 28);
    searchRegexBtn_->setToolTip("Use Regular Expression");
    searchRegexBtn_->setFocusPolicy(Qt::NoFocus);
    searchRegexBtn_->setStyleSheet(filterBtnStyle);
    connect(searchRegexBtn_, &QPushButton::toggled, this, &MainWindow::onSearchTriggered);
    searchLayout->addWidget(searchRegexBtn_);

    topBarLayout_->addWidget(searchContainer);

    // Initial populate of modelSelector_
    onEngineChanged(engineSelector_->currentIndex());

    topBarLayout_->addStretch();

    memoryLabel_ = new QLabel("RAM USED: -- MB", topBar_);
    memoryLabel_->setStyleSheet("color: #10a37f; font-weight: bold; background: #222; border-radius: 4px; padding: 2px 8px; border: 1px solid #10a37f;");
    topBarLayout_->addWidget(memoryLabel_);

    topBarLayout_->addSpacing(8);

    aboutBtn_ = new QPushButton("About", topBar_);
    aboutBtn_->setObjectName("aboutTopBtn");
    aboutBtn_->setToolTip("About ULWAI AI Agent & EG1");
    aboutBtn_->setFocusPolicy(Qt::NoFocus);
    aboutBtn_->setStyleSheet("QPushButton { background-color: #202024; color: #a1a1aa; border: 1px solid #383840; border-radius: 6px; padding: 3px 10px; font-size: 12px; font-weight: 500; } QPushButton:hover { background-color: #2d2d34; color: #ffffff; border: 1px solid #10a37f; }");
    connect(aboutBtn_, &QPushButton::clicked, this, &MainWindow::onAboutRequested);
    topBarLayout_->addWidget(aboutBtn_);

    rootLayout->addWidget(topBar_);

    // Main Splitter (Sidebar + Chat Area)
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    sidebar_ = new SessionSidebar(conversation_, splitter);
    connect(sidebar_, &SessionSidebar::sessionSelected, this, &MainWindow::onSessionSelected);
    connect(sidebar_, &SessionSidebar::newChatRequested, this, &MainWindow::onNewChatRequested);
    connect(sidebar_, &SessionSidebar::settingsRequested, this, &MainWindow::onSettingsRequested);
    connect(sidebar_, &SessionSidebar::exportRequested, this, &MainWindow::onExportRequested);
    splitter->addWidget(sidebar_);

    auto* chatContainer = new QWidget(splitter);
    auto* chatLayout = new QVBoxLayout(chatContainer);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    chatView_ = new ChatBubbleView(chatContainer);
    connect(chatView_, &ChatBubbleView::shellCommandConfirmed, this, &MainWindow::onShellCommandConfirmed);
    connect(chatView_, &ChatBubbleView::codeCopied, this, [this](const QString& snippet) {
        int lines = snippet.count('\n') + 1;
        statusBar()->showMessage(QString("✔ Copied %1 line%2 of code to clipboard!")
                                     .arg(lines)
                                     .arg(lines > 1 ? "s" : ""), 3000);
    });
    chatLayout->addWidget(chatView_, 1);

    // Keyboard Shortcuts for Font Zoom (Scales both left sidebar and right chat)
    auto* zoomInAct = new QAction(this);
    zoomInAct->setShortcuts({QKeySequence::ZoomIn, QKeySequence(Qt::CTRL | Qt::Key_Equal)});
    connect(zoomInAct, &QAction::triggered, this, [this]() { applyZoom(config_.font_size + 1, false); });
    addAction(zoomInAct);

    auto* zoomOutAct = new QAction(this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAct, &QAction::triggered, this, [this]() { applyZoom(config_.font_size - 1, false); });
    addAction(zoomOutAct);

    auto* zoomResetAct = new QAction(this);
    zoomResetAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(zoomResetAct, &QAction::triggered, this, [this]() { applyZoom(13, false); });
    addAction(zoomResetAct);

    // Bottom Input Box
    auto* inputContainer = new QWidget(chatContainer);
    inputContainer->setObjectName("inputContainer");
    auto* inputLayout = new QVBoxLayout(inputContainer);
    inputLayout->setContentsMargins(16, 8, 16, 12);
    inputLayout->setSpacing(8);

    // Control row: Modes + Attach File
    auto* controlRow = new QHBoxLayout();
    modeChatRadio_ = new QRadioButton("Chat", inputContainer);
    modeCodeRadio_ = new QRadioButton("Code Mode (-c)", inputContainer);
    modeShellRadio_ = new QRadioButton("Shell Agent (-s)", inputContainer);
    modeChatRadio_->setFocusPolicy(Qt::NoFocus);
    modeCodeRadio_->setFocusPolicy(Qt::NoFocus);
    modeShellRadio_->setFocusPolicy(Qt::NoFocus);
    modeChatRadio_->setChecked(true);

    controlRow->addWidget(modeChatRadio_);
    controlRow->addWidget(modeCodeRadio_);
    controlRow->addWidget(modeShellRadio_);

    controlRow->addSpacing(15);

    auto* attachBtn = new QPushButton("📎 Attach File", inputContainer);
    attachBtn->setObjectName("iconBtn");
    attachBtn->setFocusPolicy(Qt::NoFocus);
    connect(attachBtn, &QPushButton::clicked, this, &MainWindow::onAttachFileClicked);
    controlRow->addWidget(attachBtn);

    attachmentLabel_ = new QLabel("", inputContainer);
    attachmentLabel_->setStyleSheet("color: #38bdf8; font-weight: bold;");
    attachmentLabel_->setVisible(false);
    controlRow->addWidget(attachmentLabel_);

    removeAttachmentBtn_ = new QPushButton("✕", inputContainer);
    removeAttachmentBtn_->setObjectName("iconBtn");
    removeAttachmentBtn_->setFixedWidth(24);
    removeAttachmentBtn_->setFocusPolicy(Qt::NoFocus);
    removeAttachmentBtn_->setVisible(false);
    connect(removeAttachmentBtn_, &QPushButton::clicked, this, &MainWindow::onRemoveAttachmentClicked);
    controlRow->addWidget(removeAttachmentBtn_);

    controlRow->addStretch();
    inputLayout->addLayout(controlRow);

    // Input Edit + Buttons
    auto* promptRow = new QHBoxLayout();
    promptRow->setSpacing(10);

    promptInput_ = new PromptInputEdit(inputContainer);
    connect(promptInput_, &PromptInputEdit::sendRequested, this, &MainWindow::onSendClicked);
    promptRow->addWidget(promptInput_, 1);

    auto* btnCol = new QVBoxLayout();
    sendBtn_ = new QPushButton("Send", inputContainer);
    sendBtn_->setObjectName("sendBtn");
    sendBtn_->setFixedHeight(36);
    sendBtn_->setFocusPolicy(Qt::NoFocus);
    connect(sendBtn_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    btnCol->addWidget(sendBtn_);

    stopBtn_ = new QPushButton("Stop", inputContainer);
    stopBtn_->setObjectName("stopBtn");
    stopBtn_->setFixedHeight(36);
    stopBtn_->setFocusPolicy(Qt::NoFocus);
    stopBtn_->setVisible(false);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    btnCol->addWidget(stopBtn_);

    promptRow->addLayout(btnCol);
    inputLayout->addLayout(promptRow);

    chatLayout->addWidget(inputContainer);
    splitter->addWidget(chatContainer);

    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setSizes({260, 940});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    rootLayout->addWidget(splitter, 1);

    statusBar()->showMessage(QString("ULWAI Agent v") + QString::fromUtf8(ulwai::getVersion().data(), static_cast<int>(ulwai::getVersion().size())) + " Ready. Max RAM budget: 100MB");
}

AgentMode MainWindow::getSelectedMode() const noexcept {
    if (modeCodeRadio_->isChecked()) return AgentMode::Code;
    if (modeShellRadio_->isChecked()) return AgentMode::Shell;
    return AgentMode::Chat;
}

void MainWindow::refreshActiveSessionView() {
    chatView_->renderSession(conversation_.currentSession());
    sidebar_->selectSession(conversation_.currentSession().id);
}

void MainWindow::onSendClicked() {
    if (isGenerating_) return;

    QString promptText = promptInput_->toPlainText().trimmed();
    if (promptText.isEmpty() && attachedFileContent_.empty()) return;

    std::string fullPrompt = promptText.toStdString();
    if (!attachedFileContent_.empty()) {
        std::string fname = std::filesystem::path(attachedFilePath_).filename().string();
        fullPrompt = "[File: " + fname + "]\n" + attachedFileContent_ + "\n\nQuery: " + fullPrompt;
        onRemoveAttachmentClicked();
    }

    AgentMode mode = getSelectedMode();
    bool isShell = (mode == AgentMode::Shell);

    conversation_.addMessage(Role::User, fullPrompt, isShell);
    promptInput_->clear();
    chatView_->appendMessage(conversation_.currentSession().messages.back());
    sidebar_->selectSession(conversation_.currentSession().id);

    std::string engine = engineSelector_->currentData().toString().toStdString();
    auto client = ConversationManager::createClient(engine, config_);
    if (!client) {
        QMessageBox::critical(this, "Error", "Failed to create client for " + QString::fromStdString(engine));
        return;
    }

    std::string sys = config_.system_prompt;
    if (mode == AgentMode::Shell) {
        sys = "You are a shell command expert. Respond with ONLY the single line raw shell command that fulfills the user request. No markdown fences, no explanations.";
    } else if (mode == AgentMode::Code && sys.empty()) {
        sys = "You are an expert programmer. Provide ONLY the code formatted inside a markdown code block with the appropriate language identifier (e.g. ```python\n...\n```). Do not include conversational explanations or introductory text.";
    }

    isGenerating_ = true;
    sendBtn_->setVisible(false);
    stopBtn_->setVisible(true);
    accumulatedStreamText_.clear();

    chatView_->startStreamingMessage();

    auto messages = conversation_.getContextMessages();
    currentWorker_ = new AiWorker(std::move(client), std::move(messages), sys, this);
    connect(currentWorker_, &AiWorker::tokenReceived, this, &MainWindow::onTokenReceived);
    connect(currentWorker_, &AiWorker::finished, this, &MainWindow::onWorkerFinished);
    connect(currentWorker_, &QThread::finished, currentWorker_, &QObject::deleteLater);
    currentWorker_->start();
}

void MainWindow::onStopClicked() {
    if (currentWorker_) {
        currentWorker_->cancel();
    }
    isGenerating_ = false;
    sendBtn_->setVisible(true);
    stopBtn_->setVisible(false);

    if (!accumulatedStreamText_.empty()) {
        bool isShell = (getSelectedMode() == AgentMode::Shell);
        bool isCode = (getSelectedMode() == AgentMode::Code);
        conversation_.addMessage(Role::Assistant, accumulatedStreamText_, isShell, "", isCode);
        chatView_->appendMessage(conversation_.currentSession().messages.back());
    }
}

void MainWindow::onTokenReceived(const QString& token) {
    accumulatedStreamText_.append(token.toStdString());
    chatView_->appendStreamingToken(token);
}

void MainWindow::onWorkerFinished(const QString& fullText, const QString& error) {
    currentWorker_ = nullptr;
    isGenerating_ = false;
    sendBtn_->setVisible(true);
    stopBtn_->setVisible(false);

    std::string response = fullText.toStdString();
    if (!error.isEmpty()) {
        response = "Error: " + error.toStdString();
    }
    if (response.empty() && !accumulatedStreamText_.empty()) {
        response = accumulatedStreamText_;
    }

    bool isShell = (getSelectedMode() == AgentMode::Shell);
    bool isCode = (getSelectedMode() == AgentMode::Code);
    conversation_.addMessage(Role::Assistant, response, isShell, "", isCode);
    chatView_->appendMessage(conversation_.currentSession().messages.back());
    sidebar_->refreshSessions();
    sidebar_->selectSession(conversation_.currentSession().id);
    updateMemoryUsageLabel();
}

void MainWindow::onAttachFileClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Attach File as Context", "", "All Files (*.*)");
    if (path.isEmpty()) return;

    attachedFilePath_ = path.toStdString();
    attachedFileContent_ = utils::readFileContents(path.toStdString());

    QString fname = QFileInfo(path).fileName();
    attachmentLabel_->setText(QString("📄 %1 (%2 KB)").arg(fname).arg(attachedFileContent_.size() / 1024 + 1));
    attachmentLabel_->setVisible(true);
    removeAttachmentBtn_->setVisible(true);
}

void MainWindow::onRemoveAttachmentClicked() {
    attachedFilePath_.clear();
    attachedFileContent_.clear();
    attachmentLabel_->clear();
    attachmentLabel_->setVisible(false);
    removeAttachmentBtn_->setVisible(false);
}

void MainWindow::onEngineChanged(int /*index*/) {
    config_.default_engine = engineSelector_->currentData().toString().toStdString();

    modelSelector_->blockSignals(true);
    modelSelector_->clear();

    if (config_.default_engine == "pollinations") {
        for (const auto& [id, label] : PollinationsClient::getAvailableModels()) {
            modelSelector_->addItem(QString::fromStdString(label), QString::fromStdString(id));
        }
        int curIdx = modelSelector_->findData(QString::fromStdString(config_.pollinations.model));
        if (curIdx >= 0) modelSelector_->setCurrentIndex(curIdx);
        modelSelector_->setEnabled(true);
    } else if (config_.default_engine == "openai") {
        modelSelector_->addItem(QString::fromStdString(config_.openai.model), QString::fromStdString(config_.openai.model));
        modelSelector_->setEnabled(true);
    } else if (config_.default_engine == "ollama") {
        modelSelector_->addItem(QString::fromStdString(config_.ollama.model), QString::fromStdString(config_.ollama.model));
        modelSelector_->setEnabled(true);
    } else {
        modelSelector_->addItem(QString::fromStdString(config_.resolveModel(config_.default_engine)), QString::fromStdString(config_.resolveModel(config_.default_engine)));
        modelSelector_->setEnabled(false);
    }
    modelSelector_->blockSignals(false);

    (void)config_.save();
    statusBar()->showMessage("Engine switched to " + QString::fromStdString(config_.default_engine), 3000);
}

void MainWindow::onModelChanged(int /*index*/) {
    if (config_.default_engine == "pollinations" && modelSelector_->currentIndex() >= 0) {
        config_.pollinations.model = modelSelector_->currentData().toString().toStdString();
        (void)config_.save();
        statusBar()->showMessage("Pollinations model set to: " + QString::fromStdString(config_.pollinations.model), 3000);
    }
}

void MainWindow::onSearchTriggered() {
    if (!sidebar_) return;

    TitleSearchOptions options;
    options.case_sensitive = searchCaseBtn_ && searchCaseBtn_->isChecked();
    options.match_whole_word = searchWholeWordBtn_ && searchWholeWordBtn_->isChecked();
    options.use_regex = searchRegexBtn_ && searchRegexBtn_->isChecked();

    QString query = searchInput_ ? searchInput_->text() : "";
    sidebar_->filterSessionsByTitle(query, options);

    if (query.trimmed().isEmpty()) {
        statusBar()->showMessage(QString("Showing all %1 chats").arg(sidebar_->totalSessionCount()), 2000);
    } else {
        int matched = sidebar_->visibleSessionCount();
        int total = sidebar_->totalSessionCount();
        statusBar()->showMessage(
            QString("Search '%1': %2 of %3 chats found")
                .arg(query)
                .arg(matched)
                .arg(total),
            3000);
    }
}

void MainWindow::onSessionSelected(const QString& sessionId) {
    if (isGenerating_) onStopClicked();
    (void)conversation_.loadSession(sessionId.toStdString());
    refreshActiveSessionView();
}

void MainWindow::onNewChatRequested() {
    if (isGenerating_) onStopClicked();
    conversation_.newSession();
    sidebar_->refreshSessions();
    refreshActiveSessionView();
}

void MainWindow::onSettingsRequested() {
    SettingsDialog dlg(config_, conversation_, this);
    if (dlg.exec() == QDialog::Accepted || dlg.sessionsChanged()) {
        int idx = engineSelector_->findData(QString::fromStdString(config_.default_engine));
        if (idx >= 0) engineSelector_->setCurrentIndex(idx);
        chatView_->setSortLatestFirst(config_.chat_sort_latest_first);
        applyZoom(config_.font_size, true);
        refreshActiveSessionView();
        sidebar_->refreshSessions();
        statusBar()->showMessage("Settings updated successfully", 3000);
    }
}

void MainWindow::onAboutRequested() {
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("About ULWAI AI Agent");
    aboutDialog.setFixedSize(580, 585);
    aboutDialog.setStyleSheet(
        "QDialog { background-color: #141417; color: #f4f4f5; font-family: system-ui, -apple-system, sans-serif; }"
        "a { color: #00ffb2; text-decoration: none; font-weight: 600; }"
        "a:hover { color: #38bdf8; text-decoration: underline; }"
    );

    auto* layout = new QVBoxLayout(&aboutDialog);
    layout->setContentsMargins(32, 24, 32, 22);
    layout->setSpacing(12);

    // Top Brand Logo
    auto* logoLbl = new QLabel(&aboutDialog);
    logoLbl->setAlignment(Qt::AlignCenter);
    bool logoLoaded = false;
    if (std::filesystem::exists("resources/icons/ulwai-logo.svg")) {
        QPixmap logoPix("resources/icons/ulwai-logo.svg");
        if (!logoPix.isNull()) {
            logoLbl->setPixmap(logoPix.scaledToHeight(52, Qt::SmoothTransformation));
            logoLoaded = true;
        }
    }
    if (!logoLoaded && std::filesystem::exists("resources/icons/ulwai-logo.png")) {
        QPixmap logoPix("resources/icons/ulwai-logo.png");
        if (!logoPix.isNull()) {
            logoLbl->setPixmap(logoPix.scaledToHeight(52, Qt::SmoothTransformation));
            logoLoaded = true;
        }
    }
    if (logoLoaded) {
        layout->addWidget(logoLbl);
    } else {
        auto* title = new QLabel("<b>ULWAI</b> <span style='color:#10a37f;'>AI AGENT</span>", &aboutDialog);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
        layout->addWidget(title);
    }

    QString ver = QString::fromUtf8(ulwai::getVersion().data(), static_cast<int>(ulwai::getVersion().size()));
    auto* verLabel = new QLabel("<span style='color:#00ffb2; font-weight: bold; background: #064e3b; padding: 4px 16px; border-radius: 12px; font-size: 13px; border: 1px solid #10b981;'>v" + ver + " &bull; Native C++20</span>", &aboutDialog);
    verLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(verLabel);

    auto* desc = new QLabel("Ultra-Lightweight C++ AI Agent for Desktop & Terminal.<br>Engineered strictly for sub-100MB RAM with native C++20 performance.", &aboutDialog);
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #a1a1aa; font-size: 14px; line-height: 1.5;");
    layout->addWidget(desc);

    layout->addSpacing(4);

    // Details Grid Frame (Two-column aligned layout with 15px font)
    auto* infoFrame = new QFrame(&aboutDialog);
    infoFrame->setObjectName("aboutCard");
    infoFrame->setStyleSheet(
        "QFrame#aboutCard { background-color: #1a1a22; border: 1px solid #2f2f3d; border-radius: 10px; }"
        "QFrame#aboutCard QLabel { border: none; background: transparent; padding: 0px; font-size: 15px; }"
    );
    auto* gridLayout = new QGridLayout(infoFrame);
    gridLayout->setContentsMargins(22, 18, 22, 18);
    gridLayout->setHorizontalSpacing(16);
    gridLayout->setVerticalSpacing(12);

    auto addRow = [&](int row, const QString& key, const QString& valHtml, bool isLink = false) {
        auto* keyLbl = new QLabel(key, infoFrame);
        keyLbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #cbd5e1;");
        keyLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        gridLayout->addWidget(keyLbl, row, 0);

        auto* valLbl = new QLabel(valHtml, infoFrame);
        valLbl->setStyleSheet("font-size: 15px; color: #f1f5f9;");
        valLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        if (isLink) {
            valLbl->setOpenExternalLinks(true);
            QPalette pal = valLbl->palette();
            pal.setColor(QPalette::Link, QColor("#00ffb2"));
            pal.setColor(QPalette::LinkVisited, QColor("#00ffb2"));
            valLbl->setPalette(pal);
        }
        gridLayout->addWidget(valLbl, row, 1);
    };

    addRow(0, "Author / Creator:", "<b>EG1 (Gautam Jangid)</b>");
    addRow(1, "Official Portal:", "<a href='https://eg1.in' style='color:#00ffb2; text-decoration:none; font-weight:600;'>https://eg1.in</a>", true);
    addRow(2, "GitHub Account:", "<a href='https://github.com/EG1DOTIN' style='color:#00ffb2; text-decoration:none; font-weight:600;'>https://github.com/EG1DOTIN</a>", true);
    addRow(3, "Source Code:", "<a href='https://github.com/EG1DOTIN/ulwai' style='color:#00ffb2; text-decoration:none; font-weight:600;'>https://github.com/EG1DOTIN/ulwai</a>", true);
    addRow(4, "License:", "<span style='color:#38bdf8; font-weight:500;'>MIT License</span> (100% Free & Open Source)");

    layout->addWidget(infoFrame);

    // Update Checker Frame
    auto* updateFrame = new QFrame(&aboutDialog);
    updateFrame->setObjectName("updateCard");
    updateFrame->setStyleSheet(
        "QFrame#updateCard { background-color: #1a1a22; border: 1px solid #2f2f3d; border-radius: 10px; }"
    );
    auto* updateLayout = new QHBoxLayout(updateFrame);
    updateLayout->setContentsMargins(18, 12, 18, 12);
    updateLayout->setSpacing(14);

    auto* checkUpdatesBtn = new QPushButton("Check for Updates", updateFrame);
    checkUpdatesBtn->setObjectName("checkUpdatesBtn");
    checkUpdatesBtn->setFocusPolicy(Qt::NoFocus);
    checkUpdatesBtn->setCursor(Qt::PointingHandCursor);
    checkUpdatesBtn->setStyleSheet(
        "QPushButton { background-color: #272730; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 7px 16px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background-color: #32323e; border-color: #10b981; color: #ffffff; }"
        "QPushButton:disabled { background-color: #1c1c22; color: #71717a; border-color: #2e2e38; }"
    );

    auto* updateStatusLbl = new QLabel("Click to verify latest release version online", updateFrame);
    updateStatusLbl->setStyleSheet("color: #a1a1aa; font-size: 13px; border: none; background: transparent;");
    updateStatusLbl->setWordWrap(true);

    auto* downloadUpdateBtn = new QPushButton("Download Update", updateFrame);
    downloadUpdateBtn->setObjectName("downloadUpdateBtn");
    downloadUpdateBtn->setFocusPolicy(Qt::NoFocus);
    downloadUpdateBtn->setCursor(Qt::PointingHandCursor);
    downloadUpdateBtn->setVisible(false);
    downloadUpdateBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10b981); color: #ffffff; border: none; border-radius: 6px; padding: 7px 16px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background: #10b981; }"
    );

    updateLayout->addWidget(checkUpdatesBtn);
    updateLayout->addWidget(updateStatusLbl, 1);
    updateLayout->addWidget(downloadUpdateBtn);

    layout->addWidget(updateFrame);

    QPointer<QDialog> dialogGuard(&aboutDialog);
    QPointer<QPushButton> checkBtnGuard(checkUpdatesBtn);
    QPointer<QLabel> statusLblGuard(updateStatusLbl);
    QPointer<QPushButton> downloadBtnGuard(downloadUpdateBtn);

    connect(checkUpdatesBtn, &QPushButton::clicked, &aboutDialog, [dialogGuard, checkBtnGuard, statusLblGuard, downloadBtnGuard]() {
        if (checkBtnGuard) {
            checkBtnGuard->setEnabled(false);
            checkBtnGuard->setText("Checking...");
        }
        if (statusLblGuard) {
            statusLblGuard->setText("<span style='color:#38bdf8;'>Querying GitHub for latest version...</span>");
        }
        if (downloadBtnGuard) {
            downloadBtnGuard->setVisible(false);
        }

        std::thread([dialogGuard, checkBtnGuard, statusLblGuard, downloadBtnGuard]() {
            auto res = ulwai::utils::checkForUpdatesOnline(5);
            if (!dialogGuard) return;

            QMetaObject::invokeMethod(dialogGuard.data(), [checkBtnGuard, statusLblGuard, downloadBtnGuard, res]() {
                if (checkBtnGuard) {
                    checkBtnGuard->setEnabled(true);
                    checkBtnGuard->setText("Check Again");
                }
                if (!statusLblGuard) return;

                if (res.success && res.update_available) {
                    statusLblGuard->setText("<span style='color:#00ffb2; font-weight:bold;'>New version v" +
                        QString::fromStdString(res.latest_version) + " is available!</span>");
                    if (downloadBtnGuard) {
                        downloadBtnGuard->setVisible(true);
                        QObject::disconnect(downloadBtnGuard.data(), nullptr, nullptr, nullptr);
                        QObject::connect(downloadBtnGuard.data(), &QPushButton::clicked, [url = res.release_url]() {
                            QDesktopServices::openUrl(QUrl(QString::fromStdString(url)));
                        });
                    }
                } else if (res.success && !res.update_available) {
                    statusLblGuard->setText("<span style='color:#10b981; font-weight: 500;'>✓ You have the latest version (v" +
                        QString::fromStdString(res.current_version) + ")</span>");
                } else {
                    statusLblGuard->setText("<span style='color:#f87171;'>Check failed: " +
                        QString::fromStdString(res.error_message) + "</span>");
                }
            }, Qt::QueuedConnection);
        }).detach();
    });

    layout->addStretch();

    auto* closeBtn = new QPushButton("Close", &aboutDialog);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { background-color: #10a37f; color: #ffffff; border: none; border-radius: 6px; padding: 9px 40px; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #0d8a6a; }");
    connect(closeBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    aboutDialog.exec();
}

void MainWindow::checkUpdatesAsync(bool silentIfCurrent) {
    std::thread([this, silentIfCurrent]() {
        auto res = ulwai::utils::checkForUpdatesOnline(4);
        if (res.success && res.update_available) {
            QMetaObject::invokeMethod(this, [this, latest = res.latest_version, url = res.release_url]() {
                showUpdateNotification(latest, url);
            }, Qt::QueuedConnection);
        } else if (!silentIfCurrent && !res.success) {
            QMetaObject::invokeMethod(this, [this, msg = res.error_message]() {
                statusBar()->showMessage("Update check: " + QString::fromStdString(msg), 5000);
            }, Qt::QueuedConnection);
        }
    }).detach();
}

void MainWindow::showUpdateNotification(const std::string& latestVer, const std::string& releaseUrl) {
    if (!updateBannerBtn_ && topBarLayout_) {
        updateBannerBtn_ = new QPushButton(topBar_);
        updateBannerBtn_->setObjectName("updateBannerBtn");
        updateBannerBtn_->setCursor(Qt::PointingHandCursor);
        updateBannerBtn_->setFocusPolicy(Qt::NoFocus);
        updateBannerBtn_->setText("Update v" + QString::fromStdString(latestVer) + " Available");
        updateBannerBtn_->setToolTip("A new version of ULWAI AI Agent is available! Click to download from GitHub.");
        updateBannerBtn_->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10b981); "
            "color: #ffffff; font-weight: bold; font-size: 12px; border-radius: 6px; padding: 4px 12px; border: 1px solid #34d399; } "
            "QPushButton:hover { background: #10b981; }"
        );

        connect(updateBannerBtn_, &QPushButton::clicked, [releaseUrl]() {
            QDesktopServices::openUrl(QUrl(QString::fromStdString(releaseUrl)));
        });

        if (aboutBtn_) {
            int aboutIndex = topBarLayout_->indexOf(aboutBtn_);
            if (aboutIndex >= 0) {
                topBarLayout_->insertWidget(aboutIndex, updateBannerBtn_);
                topBarLayout_->insertSpacing(aboutIndex + 1, 8);
            } else {
                topBarLayout_->addWidget(updateBannerBtn_);
            }
        } else {
            topBarLayout_->addWidget(updateBannerBtn_);
        }
    }

    statusBar()->showMessage("ULWAI update v" + QString::fromStdString(latestVer) + " is available! Click the update badge to download.", 10000);
}

void MainWindow::onExportRequested() {
    QString filter = "Markdown File (*.md);;HTML Document (*.html)";
    QString path = QFileDialog::getSaveFileName(this, "Export Conversation", "chat_export.md", filter);
    if (path.isEmpty()) return;

    std::string content;
    if (path.endsWith(".html", Qt::CaseInsensitive)) {
        content = conversation_.exportToHtml();
    } else {
        content = conversation_.exportToMarkdown();
    }

    std::ofstream out(path.toStdString());
    if (out.is_open()) {
        out << content;
        statusBar()->showMessage("Conversation exported to " + path, 4000);
    } else {
        QMessageBox::warning(this, "Export Failed", "Could not write to file: " + path);
    }
}

void MainWindow::onShellCommandConfirmed(const QString& command) {
    auto reply = QMessageBox::question(
        this,
        "Confirm Shell Command Execution",
        QString("Are you sure you want to execute this command on your machine?\n\n%1").arg(command),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        std::string output;
        int status = platform::executeCommand(command.toStdString(), output);
        QString resultText = QString("Executed with status %1:\n%2").arg(status).arg(QString::fromStdString(output));
        conversation_.addMessage(Role::Assistant, command.toStdString(), true, output);
        refreshActiveSessionView();
    }
}

void MainWindow::updateMemoryUsageLabel() {
    long memMb = 0;
#if defined(__linux__)
    malloc_trim(0);
    std::ifstream statm("/proc/self/statm");
    long size, resident, share, text, lib, data, dt;
    if (statm >> size >> resident >> share >> text >> lib >> data >> dt) {
        long pageSizeKb = sysconf(_SC_PAGE_SIZE) / 1024;
        memMb = (resident * pageSizeKb) / 1024;
    }
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        memMb = pmc.WorkingSetSize / (1024 * 1024);
    }
#endif

    if (memMb > 0) {
        memoryLabel_->setText(QString("RAM USED: %1 MB").arg(memMb));
        if (memMb >= 100) {
            memoryLabel_->setStyleSheet("color: #ef4444; font-weight: bold; background: #222; border-radius: 4px; padding: 2px 8px; border: 1px solid #ef4444;");
        } else if (memMb >= 80) {
            memoryLabel_->setStyleSheet("color: #f59e0b; font-weight: bold; background: #222; border-radius: 4px; padding: 2px 8px; border: 1px solid #f59e0b;");
        } else {
            memoryLabel_->setStyleSheet("color: #10a37f; font-weight: bold; background: #222; border-radius: 4px; padding: 2px 8px; border: 1px solid #10a37f;");
        }
    }
}

} // namespace ulwai::ui
