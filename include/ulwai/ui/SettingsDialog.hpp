/**
 * @file SettingsDialog.hpp
 * @brief Modal Settings Dialog for Engine Keys, Endpoints & Parameters Interface.
 */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QCheckBox>
#include "ulwai/Config.hpp"
#include "ulwai/ConversationManager.hpp"

namespace ulwai::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(Config& config, ConversationManager& conversation, QWidget* parent = nullptr);

    [[nodiscard]] bool sessionsChanged() const noexcept { return sessionsChanged_; }

private slots:
    void onSaveClicked();
    void onEngineSelected(int index);
    void onImportConversationsClicked();
    void onExportConversationsClicked();
    void onDeleteAllHistoryClicked();

private:
    Config& config_;
    ConversationManager& conversation_;
    bool sessionsChanged_{false};

    QComboBox* engineCombo_{nullptr};
    QComboBox* pollinationsModel_{nullptr};
    QLineEdit* pollinationsEndpoint_{nullptr};

    QLineEdit* openaiKey_{nullptr};
    QLineEdit* openaiModel_{nullptr};
    QLineEdit* openaiEndpoint_{nullptr};

    QLineEdit* ollamaEndpoint_{nullptr};
    QLineEdit* ollamaModel_{nullptr};

    QLineEdit* openrouterKey_{nullptr};
    QLineEdit* openrouterModel_{nullptr};

    QLineEdit* grokKey_{nullptr};
    QLineEdit* grokModel_{nullptr};

    QLineEdit* hfKey_{nullptr};
    QLineEdit* hfModel_{nullptr};

    QTextEdit* systemPromptEdit_{nullptr};
    QSlider* tempSlider_{nullptr};
    QSpinBox* maxTokensBox_{nullptr};
    QSpinBox* historyLimitBox_{nullptr};
    QCheckBox* streamCheckBox_{nullptr};

    QSpinBox* fontSizeBox_{nullptr};
    QPushButton* zoomInBtn_{nullptr};
    QPushButton* zoomOutBtn_{nullptr};
    QPushButton* zoomResetBtn_{nullptr};
    QComboBox* chatOrderCombo_{nullptr};

    void setupUi();
    void loadValues();
};

} // namespace ulwai::ui
