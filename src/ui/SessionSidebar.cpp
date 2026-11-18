/**
 * @file SessionSidebar.cpp
 * @brief Collapsible Conversation History Navigation & Session Management Sidebar.
 *
 * PURPOSE & ARCHITECTURAL ROLE:
 *   Implements the left-side navigation panel for the desktop interface.
 *   Allows users to start new chat threads, navigate through stored sessions,
 *   rename titles, delete sessions, and access settings or export dialogs.
 *
 * FEATURES & SIGNALS:
 *   1. "＋ New Chat" Quick Action:
 *      - Instantly resets active context and starts a fresh conversation thread.
 *   2. Session List & Context Menu:
 *      - Populates recent conversations sorted by modification time.
 *      - Right-click context menu provides "Rename" and "Delete" actions.
 *   3. Emitted Qt Signals:
 *      - `sessionSelected(const std::string& id)`: Signals `MainWindow` to switch active conversation.
 *      - `newChatRequested()`: Signals initiation of a new blank session.
 *      - `settingsRequested()`: Opens the provider configuration modal.
 *      - `exportRequested()`: Opens the file export dialog.
 */

#include "ulwai/ui/SessionSidebar.hpp"
#include "ulwai/Utils.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>

namespace ulwai::ui {

SessionSidebar::SessionSidebar(ConversationManager& conversation, QWidget* parent)
    : QWidget(parent), conversation_(conversation)
{
    setObjectName("sidebarWidget");
    setMinimumWidth(220);
    setMaximumWidth(320);
    setupUi();
    refreshSessions();
}

void SessionSidebar::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 12, 10, 12);
    layout->setSpacing(10);

    newChatBtn_ = new QPushButton("＋  New Chat", this);
    newChatBtn_->setObjectName("newChatBtn");
    newChatBtn_->setFocusPolicy(Qt::NoFocus);
    connect(newChatBtn_, &QPushButton::clicked, this, &SessionSidebar::onNewChatClicked);
    layout->addWidget(newChatBtn_);

    sessionList_ = new QListWidget(this);
    sessionList_->setObjectName("sessionList");
    sessionList_->setContextMenuPolicy(Qt::CustomContextMenu);
    sessionList_->setFocusPolicy(Qt::NoFocus);
    sessionList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sessionList_->setTextElideMode(Qt::ElideRight);
    sessionList_->setWordWrap(false);
    sessionList_->setAutoScroll(false);
    sessionList_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    sessionList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(sessionList_, &QListWidget::itemClicked, this, &SessionSidebar::onItemClicked);
    connect(sessionList_, &QListWidget::customContextMenuRequested, this, &SessionSidebar::showContextMenu);
    layout->addWidget(sessionList_, 1);

    // Bottom tools
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(6);

    settingsBtn_ = new QPushButton("⚙ Settings", this);
    settingsBtn_->setObjectName("iconBtn");
    settingsBtn_->setFocusPolicy(Qt::NoFocus);
    connect(settingsBtn_, &QPushButton::clicked, this, &SessionSidebar::settingsRequested);
    bottomLayout->addWidget(settingsBtn_);

    exportBtn_ = new QPushButton("⤓ Export", this);
    exportBtn_->setObjectName("iconBtn");
    exportBtn_->setFocusPolicy(Qt::NoFocus);
    connect(exportBtn_, &QPushButton::clicked, this, &SessionSidebar::exportRequested);
    bottomLayout->addWidget(exportBtn_);

    layout->addLayout(bottomLayout);
}

void SessionSidebar::refreshSessions() {
    sessionList_->clear();
    auto list = conversation_.listSessions();
    for (const auto& meta : list) {
        auto* item = new QListWidgetItem();
        QString title = QString::fromStdString(meta.title).trimmed();
        title.replace('\n', ' ').replace('\r', ' ');
        if (title.isEmpty()) title = "New Conversation";
        item->setText(title);
        item->setData(Qt::UserRole, QString::fromStdString(meta.id));
        item->setToolTip(QString("Title: %1\nID: %2\nUpdated: %3\nEngine: %4")
                             .arg(title)
                             .arg(QString::fromStdString(meta.id))
                             .arg(QString::fromStdString(meta.updated_at))
                             .arg(QString::fromStdString(meta.engine)));
        sessionList_->addItem(item);
    }
    if (!currentFilterQuery_.isEmpty()) {
        filterSessionsByTitle(currentFilterQuery_, currentFilterOptions_);
    }
    if (sessionList_->horizontalScrollBar()) {
        sessionList_->horizontalScrollBar()->setValue(0);
    }
}

void SessionSidebar::filterSessionsByTitle(const QString& query, const TitleSearchOptions& options) {
    currentFilterQuery_ = query;
    currentFilterOptions_ = options;

    std::string q = query.toStdString();
    for (int i = 0; i < sessionList_->count(); ++i) {
        auto* item = sessionList_->item(i);
        std::string title = item->text().toStdString();
        bool match = utils::matchesTitle(title, q, options);
        item->setHidden(!match);
    }
}

void SessionSidebar::clearFilter() {
    currentFilterQuery_.clear();
    currentFilterOptions_ = {};
    for (int i = 0; i < sessionList_->count(); ++i) {
        sessionList_->item(i)->setHidden(false);
    }
}

int SessionSidebar::totalSessionCount() const noexcept {
    return sessionList_ ? sessionList_->count() : 0;
}

int SessionSidebar::visibleSessionCount() const noexcept {
    if (!sessionList_) return 0;
    int count = 0;
    for (int i = 0; i < sessionList_->count(); ++i) {
        if (!sessionList_->item(i)->isHidden()) {
            count++;
        }
    }
    return count;
}

void SessionSidebar::selectSession(const std::string& sessionId) {
    for (int i = 0; i < sessionList_->count(); ++i) {
        auto* item = sessionList_->item(i);
        if (item->data(Qt::UserRole).toString().toStdString() == sessionId) {
            sessionList_->setCurrentItem(item);
            if (sessionList_->horizontalScrollBar()) {
                sessionList_->horizontalScrollBar()->setValue(0);
            }
            break;
        }
    }
}

void SessionSidebar::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    emit sessionSelected(id);
}

void SessionSidebar::onNewChatClicked() {
    emit newChatRequested();
}

void SessionSidebar::showContextMenu(const QPoint& pos) {
    auto* item = sessionList_->itemAt(pos);
    if (!item) return;

    QString sessionId = item->data(Qt::UserRole).toString();

    QMenu menu(this);
    QAction* deleteAct = menu.addAction("Delete Chat");

    QAction* selected = menu.exec(sessionList_->mapToGlobal(pos));
    if (selected == deleteAct) {
        (void)conversation_.deleteSession(sessionId.toStdString());
        refreshSessions();
        emit newChatRequested();
    }
}

void SessionSidebar::updateFontSize(int size) {
    QFont f = font();
    f.setPixelSize(size);
    setFont(f);
    if (newChatBtn_) newChatBtn_->setFont(f);
    if (sessionList_) sessionList_->setFont(f);
    if (settingsBtn_) settingsBtn_->setFont(f);
    if (exportBtn_) exportBtn_->setFont(f);
}

} // namespace ulwai::ui
