/**
 * @file SettingsDialog.cpp
 * @brief Modal Settings Dialog for Engine Keys, Endpoints & User Preferences.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Provides an intuitive, tabbed configuration interface in the desktop GUI.
 *   Allows users to inspect and modify all application settings, credentials, endpoints,
 *   and model parameters without manual JSON file editing.
 *
 * TAB STRUCTURE & FEATURES:
 *   1. "Providers & Models" Tab:
 *      - Engine selector (Pollinations, DuckDuckGo, Ollama, OpenAI, OpenRouter, Grok, Hugging Face).
 *      - Per-engine credential inputs with password masking for API keys.
 *      - Endpoint and model configuration fields.
 *   2. "Generation Parameters" Tab:
 *      - Temperature slider (0.0 to 1.5), Max Tokens, Context History Turn Limit, and Token Streaming toggle.
 *   3. "System Prompt" Tab:
 *      - Custom system prompt editor to define default AI persona or operational rules.
 *   4. "Data & Storage" Tab:
 *      - Displays exact config and history paths on disk with one-click export/import and history cleanup.
 */

#include "ulwai/ui/SettingsDialog.hpp"
#include "ulwai/ui/UiStyles.hpp"
#include "ulwai/PollinationsClient.hpp"
#include "ulwai/Utils.hpp"
#include "ulwai/Version.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QTabBar>
#include <fstream>

namespace ulwai::ui {

SettingsDialog::SettingsDialog(Config& config, ConversationManager& conversation, QWidget* parent)
    : QDialog(parent), config_(config), conversation_(conversation)
{
    QString ver = QString::fromUtf8(ulwai::getVersion().data(), static_cast<int>(ulwai::getVersion().size()));
    setWindowTitle("ULWAI Agent Settings v" + ver);
    resize(640, 620);
    setStyleSheet(QString::fromStdString(getSettingsDialogStylesheet(config_.font_size)));
    setupUi();
    loadValues();
}

void SettingsDialog::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* mainFrame = new QFrame(this);
    mainFrame->setObjectName("settingsRootFrame");
    rootLayout->addWidget(mainFrame);

    auto* mainLayout = new QVBoxLayout(mainFrame);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    auto* tabs = new QTabWidget(mainFrame);
    tabs->setFocusPolicy(Qt::NoFocus);
    if (tabs->tabBar()) {
        tabs->tabBar()->setFocusPolicy(Qt::NoFocus);
        tabs->tabBar()->setElideMode(Qt::ElideNone);
    }

    // Tab 1: Providers & Models
    auto* providerTab = new QWidget();
    auto* providerLayout = new QVBoxLayout(providerTab);
    providerLayout->setContentsMargins(14, 14, 14, 14);
    providerLayout->setSpacing(16);

    auto* engineRow = new QHBoxLayout();
    engineRow->addWidget(new QLabel("Default AI Engine:", providerTab));
    engineCombo_ = new QComboBox(providerTab);
    engineCombo_->addItem("Pollinations (Free AI / No Key)", "pollinations");
    engineCombo_->addItem("DuckDuckGo (Search / Facts)", "duckduckgo");
    engineCombo_->addItem("Ollama (Local Offline)", "ollama");
    engineCombo_->addItem("OpenAI (or Compatible)", "openai");
    engineCombo_->addItem("OpenRouter", "openrouter");
    engineCombo_->addItem("xAI Grok", "grok");
    engineCombo_->addItem("Hugging Face", "huggingface");
    engineRow->addWidget(engineCombo_);
    providerLayout->addLayout(engineRow);

    // Pollinations Group
    auto* pollGroup = new QGroupBox("Pollinations.ai (Free Conversational AI)", providerTab);
    auto* pollForm = new QFormLayout(pollGroup);
    pollForm->setContentsMargins(16, 20, 16, 16);
    pollForm->setSpacing(12);
    pollinationsModel_ = new QComboBox(pollGroup);
    for (const auto& [id, label] : PollinationsClient::getAvailableModels()) {
        pollinationsModel_->addItem(QString::fromStdString(label), QString::fromStdString(id));
    }
    pollinationsEndpoint_ = new QLineEdit(pollGroup);
    pollinationsEndpoint_->setPlaceholderText("https://text.pollinations.ai/openai");
    pollForm->addRow("Model:", pollinationsModel_);
    pollForm->addRow("Endpoint:", pollinationsEndpoint_);
    providerLayout->addWidget(pollGroup);

    // OpenAI Group
    auto* openaiGroup = new QGroupBox("OpenAI / Custom Compatible Server", providerTab);
    auto* openaiForm = new QFormLayout(openaiGroup);
    openaiForm->setContentsMargins(16, 20, 16, 16);
    openaiForm->setSpacing(12);
    openaiKey_ = new QLineEdit(openaiGroup);
    openaiKey_->setEchoMode(QLineEdit::Password);
    openaiKey_->setPlaceholderText("sk-...");
    openaiModel_ = new QLineEdit(openaiGroup);
    openaiModel_->setPlaceholderText("gpt-4o-mini");
    openaiEndpoint_ = new QLineEdit(openaiGroup);
    openaiEndpoint_->setPlaceholderText("https://api.openai.com/v1/chat/completions");
    openaiForm->addRow("API Key:", openaiKey_);
    openaiForm->addRow("Model:", openaiModel_);
    openaiForm->addRow("Endpoint:", openaiEndpoint_);
    providerLayout->addWidget(openaiGroup);

    // Ollama Group
    auto* ollamaGroup = new QGroupBox("Ollama / Local LLM Server", providerTab);
    auto* ollamaForm = new QFormLayout(ollamaGroup);
    ollamaForm->setContentsMargins(16, 20, 16, 16);
    ollamaForm->setSpacing(12);
    ollamaEndpoint_ = new QLineEdit(ollamaGroup);
    ollamaEndpoint_->setPlaceholderText("http://localhost:11434/api/chat");
    ollamaModel_ = new QLineEdit(ollamaGroup);
    ollamaModel_->setPlaceholderText("llama3");
    ollamaForm->addRow("API Endpoint:", ollamaEndpoint_);
    ollamaForm->addRow("Model Name:", ollamaModel_);
    providerLayout->addWidget(ollamaGroup);

    // OpenRouter & Others
    auto* otherGroup = new QGroupBox("OpenRouter / Grok / Hugging Face", providerTab);
    auto* otherForm = new QFormLayout(otherGroup);
    otherForm->setContentsMargins(16, 20, 16, 16);
    otherForm->setSpacing(12);
    openrouterKey_ = new QLineEdit(otherGroup);
    openrouterKey_->setEchoMode(QLineEdit::Password);
    openrouterModel_ = new QLineEdit(otherGroup);
    openrouterModel_->setPlaceholderText("meta-llama/llama-3.1-8b-instruct:free");
    otherForm->addRow("OpenRouter Key:", openrouterKey_);
    otherForm->addRow("OpenRouter Model:", openrouterModel_);

    grokKey_ = new QLineEdit(otherGroup);
    grokKey_->setEchoMode(QLineEdit::Password);
    grokModel_ = new QLineEdit(otherGroup);
    otherForm->addRow("xAI Grok Key:", grokKey_);
    otherForm->addRow("Grok Model:", grokModel_);

    hfKey_ = new QLineEdit(otherGroup);
    hfKey_->setEchoMode(QLineEdit::Password);
    hfModel_ = new QLineEdit(otherGroup);
    otherForm->addRow("HuggingFace Key:", hfKey_);
    otherForm->addRow("HF Model:", hfModel_);
    providerLayout->addWidget(otherGroup);

    auto* providerScroll = new QScrollArea(tabs);
    providerScroll->setWidgetResizable(true);
    providerScroll->setFrameShape(QFrame::NoFrame);
    providerScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    providerScroll->setWidget(providerTab);
    tabs->addTab(providerScroll, "Providers");

    // Tab 2: Agent Behavior
    auto* behaviorTab = new QWidget();
    auto* behaviorLayout = new QVBoxLayout(behaviorTab);
    behaviorLayout->setContentsMargins(14, 14, 14, 14);
    behaviorLayout->setSpacing(16);

    behaviorLayout->addWidget(new QLabel("Custom Instructions / System Prompt:", behaviorTab));
    systemPromptEdit_ = new QTextEdit(behaviorTab);
    systemPromptEdit_->setPlaceholderText("You are an expert AI assistant...");
    systemPromptEdit_->setMaximumHeight(120);
    behaviorLayout->addWidget(systemPromptEdit_);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(14);
    auto* tempLayout = new QHBoxLayout();
    tempSlider_ = new QSlider(Qt::Horizontal, behaviorTab);
    tempSlider_->setRange(0, 200);
    tempSlider_->setValue(70);
    auto* tempValLabel = new QLabel("0.7", behaviorTab);
    connect(tempSlider_, &QSlider::valueChanged, this, [tempValLabel](int val) {
        tempValLabel->setText(QString::number(val / 100.0, 'f', 2));
    });
    tempLayout->addWidget(tempSlider_);
    tempLayout->addWidget(tempValLabel);
    formLayout->addRow("Temperature:", tempLayout);

    maxTokensBox_ = new QSpinBox(behaviorTab);
    maxTokensBox_->setRange(64, 32768);
    maxTokensBox_->setValue(2048);
    maxTokensBox_->setSingleStep(256);
    formLayout->addRow("Max Output Tokens:", maxTokensBox_);

    historyLimitBox_ = new QSpinBox(behaviorTab);
    historyLimitBox_->setRange(1, 100);
    historyLimitBox_->setValue(20);
    formLayout->addRow("Chat History Memory Limit:", historyLimitBox_);

    streamCheckBox_ = new QCheckBox("Stream tokens in real-time", behaviorTab);
    streamCheckBox_->setChecked(true);
    formLayout->addRow("Live Streaming:", streamCheckBox_);

    // Font size / Zoom in / Zoom out controls
    auto* zoomRow = new QHBoxLayout();
    zoomRow->setSpacing(8);
    zoomOutBtn_ = new QPushButton("－ Zoom Out", behaviorTab);
    zoomOutBtn_->setObjectName("actionBtn");
    zoomOutBtn_->setFocusPolicy(Qt::NoFocus);

    fontSizeBox_ = new QSpinBox(behaviorTab);
    fontSizeBox_->setRange(9, 32);
    fontSizeBox_->setValue(13);
    fontSizeBox_->setSuffix(" px");

    zoomInBtn_ = new QPushButton("＋ Zoom In", behaviorTab);
    zoomInBtn_->setObjectName("actionBtn");
    zoomInBtn_->setFocusPolicy(Qt::NoFocus);

    zoomResetBtn_ = new QPushButton("Reset", behaviorTab);
    zoomResetBtn_->setObjectName("actionBtn");
    zoomResetBtn_->setFocusPolicy(Qt::NoFocus);

    connect(zoomInBtn_, &QPushButton::clicked, this, [this]() {
        fontSizeBox_->setValue(fontSizeBox_->value() + 1);
    });
    connect(zoomOutBtn_, &QPushButton::clicked, this, [this]() {
        fontSizeBox_->setValue(fontSizeBox_->value() - 1);
    });
    connect(zoomResetBtn_, &QPushButton::clicked, this, [this]() {
        fontSizeBox_->setValue(13);
    });

    zoomRow->addWidget(zoomOutBtn_);
    zoomRow->addWidget(fontSizeBox_);
    zoomRow->addWidget(zoomInBtn_);
    zoomRow->addWidget(zoomResetBtn_);
    formLayout->addRow("Font Size (Zoom):", zoomRow);

    // Chat Message Ordering
    chatOrderCombo_ = new QComboBox(behaviorTab);
    chatOrderCombo_->addItem("Chronological (User first, then AI – like ChatGPT/Gemini)", false);
    chatOrderCombo_->addItem("Latest on top (Newest first)", true);
    formLayout->addRow("Message Order:", chatOrderCombo_);

    behaviorLayout->addLayout(formLayout);

    auto* behaviorScroll = new QScrollArea(tabs);
    behaviorScroll->setWidgetResizable(true);
    behaviorScroll->setFrameShape(QFrame::NoFrame);
    behaviorScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    behaviorScroll->setWidget(behaviorTab);
    tabs->addTab(behaviorScroll, "Parameters");

    // Tab 3: Data & Storage
    auto* dataTab = new QWidget();
    auto* dataLayout = new QVBoxLayout(dataTab);
    dataLayout->setSpacing(20);
    dataLayout->setContentsMargins(16, 16, 16, 16);

    // Group 1: Backup & Migration
    auto* backupGroup = new QGroupBox("Conversation Backup && Migration", dataTab);
    auto* backupLayout = new QVBoxLayout(backupGroup);
    backupLayout->setContentsMargins(16, 20, 16, 16);
    backupLayout->setSpacing(14);

    auto* backupDesc = new QLabel(
        "Import or export conversations using universal <b>conversations.json</b> format (compatible with ChatGPT, LibreChat, and ULWAI).", backupGroup);
    backupDesc->setObjectName("descLabel");
    backupDesc->setWordWrap(true);
    backupLayout->addWidget(backupDesc);

    auto* ioRow = new QHBoxLayout();
    ioRow->setSpacing(12);
    auto* importBtn = new QPushButton("📥  Import conversations.json", backupGroup);
    importBtn->setObjectName("actionBtn");
    importBtn->setFocusPolicy(Qt::NoFocus);
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportConversationsClicked);
    ioRow->addWidget(importBtn);

    auto* exportBtn = new QPushButton("📤  Export All (conversations.json)", backupGroup);
    exportBtn->setObjectName("actionBtn");
    exportBtn->setFocusPolicy(Qt::NoFocus);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportConversationsClicked);
    ioRow->addWidget(exportBtn);

    backupLayout->addLayout(ioRow);
    dataLayout->addWidget(backupGroup);

    // Group 2: Danger Zone
    auto* dangerGroup = new QGroupBox("Danger Zone", dataTab);
    dangerGroup->setObjectName("dangerGroup");
    auto* dangerLayout = new QVBoxLayout(dangerGroup);
    dangerLayout->setContentsMargins(16, 20, 16, 16);
    dangerLayout->setSpacing(14);

    auto* dangerDesc = new QLabel(
        "Permanently clears all saved conversations from the application. We recommend creating an export backup first.", dangerGroup);
    dangerDesc->setObjectName("dangerDescLabel");
    dangerDesc->setWordWrap(true);
    dangerLayout->addWidget(dangerDesc);

    auto* deleteAllBtn = new QPushButton("🗑  Delete All Chat History", dangerGroup);
    deleteAllBtn->setObjectName("dangerBtn");
    deleteAllBtn->setFocusPolicy(Qt::NoFocus);
    connect(deleteAllBtn, &QPushButton::clicked, this, &SettingsDialog::onDeleteAllHistoryClicked);
    dangerLayout->addWidget(deleteAllBtn);

    dataLayout->addWidget(dangerGroup);
    dataLayout->addStretch();

    tabs->addTab(dataTab, "Data && Storage");

    mainLayout->addWidget(tabs);

    // Bottom buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();
    auto* cancelBtn = new QPushButton("Cancel", mainFrame);
    cancelBtn->setObjectName("secondaryBtn");
    cancelBtn->setFocusPolicy(Qt::NoFocus);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    auto* saveBtn = new QPushButton("Save Settings", mainFrame);
    saveBtn->setObjectName("primaryBtn");
    saveBtn->setFocusPolicy(Qt::NoFocus);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);
}

void SettingsDialog::loadValues() {
    int idx = engineCombo_->findData(QString::fromStdString(config_.default_engine));
    if (idx >= 0) engineCombo_->setCurrentIndex(idx);

    int pIdx = pollinationsModel_->findData(QString::fromStdString(config_.pollinations.model));
    if (pIdx >= 0) pollinationsModel_->setCurrentIndex(pIdx);
    pollinationsEndpoint_->setText(QString::fromStdString(config_.pollinations.endpoint));

    openaiKey_->setText(QString::fromStdString(config_.openai.api_key));
    openaiModel_->setText(QString::fromStdString(config_.openai.model));
    openaiEndpoint_->setText(QString::fromStdString(config_.openai.endpoint));

    ollamaEndpoint_->setText(QString::fromStdString(config_.ollama.endpoint));
    ollamaModel_->setText(QString::fromStdString(config_.ollama.model));

    openrouterKey_->setText(QString::fromStdString(config_.openrouter.api_key));
    openrouterModel_->setText(QString::fromStdString(config_.openrouter.model));

    grokKey_->setText(QString::fromStdString(config_.grok.api_key));
    grokModel_->setText(QString::fromStdString(config_.grok.model));

    hfKey_->setText(QString::fromStdString(config_.huggingface.api_key));
    hfModel_->setText(QString::fromStdString(config_.huggingface.model));

    systemPromptEdit_->setPlainText(QString::fromStdString(config_.system_prompt));
    tempSlider_->setValue(static_cast<int>(config_.temperature * 100.0));
    maxTokensBox_->setValue(config_.max_tokens);
    historyLimitBox_->setValue(config_.history_limit);
    streamCheckBox_->setChecked(config_.stream_output);

    fontSizeBox_->setValue(config_.font_size);
    int orderIdx = chatOrderCombo_->findData(config_.chat_sort_latest_first);
    if (orderIdx >= 0) chatOrderCombo_->setCurrentIndex(orderIdx);
}

void SettingsDialog::onEngineSelected(int /*index*/) {}

void SettingsDialog::onSaveClicked() {
    config_.default_engine = engineCombo_->currentData().toString().toStdString();

    config_.pollinations.model = pollinationsModel_->currentData().toString().toStdString();
    config_.pollinations.endpoint = pollinationsEndpoint_->text().trimmed().toStdString();

    config_.openai.api_key = openaiKey_->text().trimmed().toStdString();
    config_.openai.model = openaiModel_->text().trimmed().toStdString();
    config_.openai.endpoint = openaiEndpoint_->text().trimmed().toStdString();

    config_.ollama.endpoint = ollamaEndpoint_->text().trimmed().toStdString();
    config_.ollama.model = ollamaModel_->text().trimmed().toStdString();

    config_.openrouter.api_key = openrouterKey_->text().trimmed().toStdString();
    config_.openrouter.model = openrouterModel_->text().trimmed().toStdString();

    config_.grok.api_key = grokKey_->text().trimmed().toStdString();
    config_.grok.model = grokModel_->text().trimmed().toStdString();

    config_.huggingface.api_key = hfKey_->text().trimmed().toStdString();
    config_.huggingface.model = hfModel_->text().trimmed().toStdString();

    config_.system_prompt = systemPromptEdit_->toPlainText().trimmed().toStdString();
    config_.temperature = tempSlider_->value() / 100.0;
    config_.max_tokens = maxTokensBox_->value();
    config_.history_limit = historyLimitBox_->value();
    config_.stream_output = streamCheckBox_->isChecked();

    config_.font_size = fontSizeBox_->value();
    config_.chat_sort_latest_first = chatOrderCombo_->currentData().toBool();

    (void)config_.save();
    accept();
}

void SettingsDialog::onImportConversationsClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Import Conversations", "", "JSON Files (*.json);;All Files (*.*)");
    if (path.isEmpty()) return;

    std::string content = utils::readFileContents(path.toStdString());
    if (content.empty()) {
        QMessageBox::warning(this, "Import Error", "The selected file is empty or could not be read.");
        return;
    }

    size_t count = conversation_.importConversationsJson(content);
    if (count > 0) {
        sessionsChanged_ = true;
        QMessageBox::information(this, "Import Successful", QString("Successfully imported %1 conversation%2 from %3!")
                                                              .arg(count)
                                                              .arg(count > 1 ? "s" : "")
                                                              .arg(QFileInfo(path).fileName()));
    } else {
        QMessageBox::warning(this, "Import Error", "No valid conversations found in the selected JSON file.\nEnsure it matches ChatGPT or LibreChat export format.");
    }
}

void SettingsDialog::onExportConversationsClicked() {
    QString path = QFileDialog::getSaveFileName(this, "Export All Conversations", "conversations.json", "JSON Files (*.json);;All Files (*.*)");
    if (path.isEmpty()) return;

    std::string jsonStr = conversation_.exportAllConversationsJson();
    std::ofstream out(path.toStdString());
    if (out.is_open()) {
        out << jsonStr;
        QMessageBox::information(this, "Export Successful", QString("Successfully exported all conversations to:\n%1").arg(path));
    } else {
        QMessageBox::critical(this, "Export Error", "Failed to write export file to disk.");
    }
}

void SettingsDialog::onDeleteAllHistoryClicked() {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Warning: Delete All Chat History");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("<b>Are you sure you want to delete all chat history?</b>");
    msgBox.setInformativeText("This will remove all saved chat sessions. We strongly suggest exporting your conversations first to avoid data loss.");

    QPushButton* exportFirstBtn = msgBox.addButton("Export All First (Recommended)", QMessageBox::ActionRole);
    QPushButton* deleteNowBtn = msgBox.addButton("Delete All Now", QMessageBox::DestructiveRole);
    QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.setDefaultButton(exportFirstBtn);

    msgBox.exec();

    if (msgBox.clickedButton() == cancelBtn || (msgBox.clickedButton() != deleteNowBtn && msgBox.clickedButton() != exportFirstBtn)) {
        return;
    }

    if (msgBox.clickedButton() == exportFirstBtn) {
        QString path = QFileDialog::getSaveFileName(this, "Export All Conversations Before Deleting", "conversations.json", "JSON Files (*.json)");
        if (!path.isEmpty()) {
            std::string jsonStr = conversation_.exportAllConversationsJson();
            std::ofstream out(path.toStdString());
            if (out.is_open()) {
                out << jsonStr;
            } else {
                QMessageBox::critical(this, "Export Error", "Failed to write export file. Deletion aborted.");
                return;
            }
        } else {
            // User cancelled export
            return;
        }

        auto proceed = QMessageBox::question(this, "Proceed with Deletion?",
            "Export saved successfully!\n\nDo you still want to delete all chat history now?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (proceed != QMessageBox::Yes) {
            return;
        }
    }

    size_t deletedCount = conversation_.deleteAllSessions();
    sessionsChanged_ = true;
    QMessageBox::information(this, "History Cleared", QString("Successfully deleted %1 conversation%2 (safely archived to trash).")
                                                         .arg(deletedCount)
                                                         .arg(deletedCount > 1 ? "s" : ""));
}

} // namespace ulwai::ui
