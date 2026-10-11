#include "ToolDetailPage.h"
#include "ConsoleWidget.h"
#include "core/PathResolver.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

namespace {

void clearLayout(QLayout *layout)
{
    if (!layout)
        return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        if (QLayout *child = item->layout()) {
            clearLayout(child);
            delete child;
        }
        delete item;
    }
}

QLabel *makeSectionTitle(const QString &text, QWidget *parent)
{
    auto *l = new QLabel(text, parent);
    l->setObjectName(QStringLiteral("SectionTitle"));
    return l;
}

QString stateBadgeText(ToolState s)
{
    switch (s) {
    case ToolState::Ok:
    case ToolState::ExternalOk:
        return QStringLiteral("● 可用");
    case ToolState::NeedCompile:
        return QStringLiteral("● 需编译");
    case ToolState::ExternalMissing:
        return QStringLiteral("● 外部路径缺失");
    case ToolState::Missing:
    default:
        return QStringLiteral("● 缺失");
    }
}

QString stateName(ToolState s)
{
    switch (s) {
    case ToolState::Ok:
    case ToolState::ExternalOk:
        return QStringLiteral("ok");
    case ToolState::NeedCompile:
        return QStringLiteral("compile");
    default:
        return QStringLiteral("missing");
    }
}

} // namespace

ToolDetailPage::ToolDetailPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void ToolDetailPage::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scroll);
    auto *col = new QVBoxLayout(content);
    col->setContentsMargins(18, 14, 18, 18);
    col->setSpacing(12);

    // 顶部：返回 + 图标 + 标题 + 徽标
    auto *topRow = new QHBoxLayout();
    auto *backBtn = new QPushButton(QStringLiteral("← 返回"), content);
    backBtn->setObjectName(QStringLiteral("GhostButton"));
    connect(backBtn, &QPushButton::clicked, this, &ToolDetailPage::backRequested);
    topRow->addWidget(backBtn, 0, Qt::AlignTop);

    m_icon = new QLabel(content);
    m_icon->setFixedSize(48, 48);
    m_icon->setAlignment(Qt::AlignCenter);
    topRow->addWidget(m_icon);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(4);
    auto *titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);
    m_title = new QLabel(content);
    m_title->setObjectName(QStringLiteral("DetailTitle"));
    m_badges = new QLabel(content);
    m_badges->setObjectName(QStringLiteral("CardBadge"));
    titleRow->addWidget(m_title);
    titleRow->addWidget(m_badges);
    titleRow->addStretch();
    titleCol->addLayout(titleRow);
    m_path = new QLabel(content);
    m_path->setObjectName(QStringLiteral("Muted"));
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    titleCol->addWidget(m_path);
    topRow->addLayout(titleCol, 1);
    col->addLayout(topRow);

    m_note = new QLabel(content);
    m_note->setObjectName(QStringLiteral("NoteBox"));
    m_note->setWordWrap(true);
    m_note->hide();
    col->addWidget(m_note);

    // 简介
    col->addWidget(makeSectionTitle(QStringLiteral("工具简介"), content));
    m_desc = new QLabel(content);
    m_desc->setObjectName(QStringLiteral("BodyText"));
    m_desc->setWordWrap(true);
    m_desc->setTextInteractionFlags(Qt::TextSelectableByMouse);
    col->addWidget(m_desc);

    // 使用说明
    col->addWidget(makeSectionTitle(QStringLiteral("使用说明"), content));
    m_usage = new QLabel(content);
    m_usage->setObjectName(QStringLiteral("BodyText"));
    m_usage->setWordWrap(true);
    m_usage->setTextFormat(Qt::RichText);
    m_usage->setTextInteractionFlags(Qt::TextSelectableByMouse);
    col->addWidget(m_usage);

    // 命令示例
    m_examplesBox = new QWidget(content);
    auto *exCol = new QVBoxLayout(m_examplesBox);
    exCol->setContentsMargins(0, 0, 0, 0);
    exCol->setSpacing(6);
    exCol->addWidget(makeSectionTitle(QStringLiteral("命令示例"), m_examplesBox));
    m_examplesLayout = new QVBoxLayout();
    m_examplesLayout->setSpacing(6);
    exCol->addLayout(m_examplesLayout);
    col->addWidget(m_examplesBox);

    // 命令大全
    m_commandsBox = new QWidget(content);
    auto *cmdCol = new QVBoxLayout(m_commandsBox);
    cmdCol->setContentsMargins(0, 0, 0, 0);
    cmdCol->setSpacing(6);
    cmdCol->addWidget(makeSectionTitle(QStringLiteral("命令大全"), m_commandsBox));
    m_commandsLayout = new QVBoxLayout();
    m_commandsLayout->setSpacing(6);
    cmdCol->addLayout(m_commandsLayout);
    col->addWidget(m_commandsBox);

    // 提示与技巧
    m_tipsBox = new QWidget(content);
    auto *tipCol = new QVBoxLayout(m_tipsBox);
    tipCol->setContentsMargins(0, 0, 0, 0);
    tipCol->setSpacing(4);
    tipCol->addWidget(makeSectionTitle(QStringLiteral("注意事项 / CTF 技巧"), m_tipsBox));
    m_tipsLayout = new QVBoxLayout();
    m_tipsLayout->setSpacing(4);
    tipCol->addLayout(m_tipsLayout);
    col->addWidget(m_tipsBox);

    // 依赖
    m_depsBox = new QWidget(content);
    auto *depCol = new QVBoxLayout(m_depsBox);
    depCol->setContentsMargins(0, 0, 0, 0);
    depCol->setSpacing(4);
    depCol->addWidget(makeSectionTitle(QStringLiteral("运行依赖"), m_depsBox));
    m_depsLayout = new QVBoxLayout();
    m_depsLayout->setSpacing(4);
    depCol->addLayout(m_depsLayout);
    col->addWidget(m_depsBox);

    // 操作区
    col->addWidget(makeSectionTitle(QStringLiteral("操作"), content));
    m_runSection = new QWidget(content);
    auto *runCol = new QVBoxLayout(m_runSection);
    runCol->setContentsMargins(0, 0, 0, 0);
    runCol->setSpacing(8);

    auto *btnRow = new QHBoxLayout();
    m_primaryBtn = new QPushButton(QStringLiteral("启动"), m_runSection);
    m_primaryBtn->setObjectName(QStringLiteral("PrimaryButton"));
    m_inputsBtn = new QPushButton(QStringLiteral("准备文件"), m_runSection);
    m_inputsBtn->setToolTip(QStringLiteral("拖拽 / 选择工具所需的前置文件，自动复制到工具目录"));
    m_stopBtn = new QPushButton(QStringLiteral("停止"), m_runSection);
    m_folderBtn = new QPushButton(QStringLiteral("打开所在目录"), m_runSection);
    m_websiteBtn = new QPushButton(QStringLiteral("访问官网"), m_runSection);
    m_inputsBtn->setObjectName(QStringLiteral("GhostButton"));
    m_stopBtn->setObjectName(QStringLiteral("GhostButton"));
    m_folderBtn->setObjectName(QStringLiteral("GhostButton"));
    m_websiteBtn->setObjectName(QStringLiteral("GhostButton"));
    btnRow->addWidget(m_primaryBtn);
    btnRow->addWidget(m_inputsBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_folderBtn);
    btnRow->addWidget(m_websiteBtn);
    runCol->addLayout(btnRow);

    m_args = new QLineEdit(m_runSection);
    m_args->setPlaceholderText(QStringLiteral("追加命令行参数（可选），例如 -d example.com"));
    runCol->addWidget(m_args);

    m_console = new ConsoleWidget(m_runSection);
    runCol->addWidget(m_console);
    col->addWidget(m_runSection);

    col->addStretch();
    scroll->setWidget(content);
    root->addWidget(scroll);

    connect(m_primaryBtn, &QPushButton::clicked, this, [this]() {
        if (!m_tool.ok() || m_id.isEmpty())
            return;
        if (m_tool.needsConsole()) {
            const QStringList extra = m_args->text().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            emit runRequested(m_id, extra);
        } else {
            emit launchRequested(m_id);
        }
    });
    connect(m_stopBtn, &QPushButton::clicked, this, &ToolDetailPage::stopRequested);
    connect(m_inputsBtn, &QPushButton::clicked, this, [this]() {
        if (!m_id.isEmpty())
            emit prepareInputsRequested(m_id);
    });
    connect(m_folderBtn, &QPushButton::clicked, this, [this]() {
        const QString dir = PathResolver::parentDir(m_tool.resolvedPath);
        emit openFolderRequested(dir);
    });
    connect(m_websiteBtn, &QPushButton::clicked, this, [this]() {
        emit openUrlRequested(m_tool.website);
    });
    connect(m_console, &ConsoleWidget::stdinSubmitted, this, &ToolDetailPage::stdinSubmitted);
}

void ToolDetailPage::setEnv(const EnvStatus &env)
{
    m_env = env;
    if (!m_id.isEmpty())
        rebuildDynamic();
}

void ToolDetailPage::loadTool(const ToolInfo &tool, const EnvStatus &env)
{
    m_tool = tool;
    m_env = env;
    m_id = tool.id;

    const QString iconPath = tool.icon.isEmpty()
                                 ? QStringLiteral(":/icons/cat_%1.svg").arg(tool.categoryId)
                                 : QStringLiteral(":/icons/%1").arg(tool.icon);
    QIcon icon(iconPath);
    if (icon.isNull())
        icon = QIcon(QStringLiteral(":/icons/app.svg"));
    m_icon->setPixmap(icon.pixmap(48, 48));

    m_title->setText(tool.name);
    m_badges->setText(stateBadgeText(tool.state));
    m_badges->setProperty("state", stateName(tool.state));
    m_badges->style()->unpolish(m_badges);
    m_badges->style()->polish(m_badges);

    m_path->setText(tool.resolvedPath.isEmpty() ? QStringLiteral("（未解析路径）") : tool.resolvedPath);
    m_desc->setText(tool.description);

    QString usage = tool.usage;
    if (!tool.tags.isEmpty())
        usage += QStringLiteral("<br><span style='color:#8b93a1'>标签：%1</span>")
                     .arg(tool.tags.join(QStringLiteral("、")));
    m_usage->setText(usage);

    // 提示信息（缺失 / 需编译）
    if (tool.state == ToolState::NeedCompile && !tool.buildNote.isEmpty()) {
        m_note->setText(QStringLiteral("⚠ 该工具当前只有源码，需自行编译后才能使用。\n%1").arg(tool.buildNote));
        m_note->show();
    } else if (tool.state == ToolState::Missing || tool.state == ToolState::ExternalMissing) {
        m_note->setText(QStringLiteral("⚠ 未找到该工具的可执行文件，请在「设置」中检查工具根目录，"
                                       "或在 user_tools.json 中修正路径。"));
        m_note->show();
    } else {
        m_note->hide();
    }

    // 主按钮文案
    if (tool.needsConsole()) {
        m_primaryBtn->setText(QStringLiteral("运行"));
        m_args->setPlaceholderText(tool.argsPlaceholder.isEmpty()
                                       ? QStringLiteral("追加命令行参数（可选）")
                                       : tool.argsPlaceholder);
        m_args->show();
        m_console->show();
    } else if (tool.type == ToolType::Html || tool.launchKind == LaunchKind::OpenUrl) {
        m_primaryBtn->setText(QStringLiteral("在浏览器中打开"));
        m_args->hide();
        m_console->hide();
    } else if (tool.type == ToolType::Folder || tool.launchKind == LaunchKind::Folder) {
        m_primaryBtn->setText(QStringLiteral("打开目录"));
        m_args->hide();
        m_console->hide();
    } else {
        m_primaryBtn->setText(QStringLiteral("启动工具"));
        m_args->hide();
        m_console->hide();
    }

    m_primaryBtn->setEnabled(tool.ok());
    m_stopBtn->setEnabled(false);
    m_folderBtn->setEnabled(!tool.resolvedPath.isEmpty());
    m_websiteBtn->setEnabled(!tool.website.isEmpty());
    m_folderBtn->setVisible(!tool.resolvedPath.isEmpty());
    m_websiteBtn->setVisible(!tool.website.isEmpty());
    m_inputsBtn->setVisible(tool.hasInputs());

    rebuildDynamic();
    clearConsole();
}

void ToolDetailPage::rebuildDynamic()
{
    // 命令示例
    clearLayout(m_examplesLayout);
    m_examplesBox->setVisible(!m_tool.examples.isEmpty());
    for (const ToolExample &ex : m_tool.examples) {
        auto *row = new QFrame(m_examplesBox);
        row->setObjectName(QStringLiteral("ExampleRow"));
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(10, 6, 10, 6);
        h->setSpacing(8);
        auto *code = new QLabel(ex.cmd, row);
        code->setObjectName(QStringLiteral("CodeText"));
        code->setTextInteractionFlags(Qt::TextSelectableByMouse);
        code->setWordWrap(true);
        auto *copy = new QPushButton(QStringLiteral("复制"), row);
        copy->setObjectName(QStringLiteral("GhostButton"));
        const QString cmd = ex.cmd;
        connect(copy, &QPushButton::clicked, this, [cmd]() {
            QApplication::clipboard()->setText(cmd);
        });
        h->addWidget(code, 1);
        h->addWidget(copy, 0, Qt::AlignTop);
        m_examplesLayout->addWidget(row);
        if (!ex.desc.isEmpty()) {
            auto *d = new QLabel(QStringLiteral("　└ %1").arg(ex.desc), m_examplesBox);
            d->setObjectName(QStringLiteral("Muted"));
            d->setWordWrap(true);
            m_examplesLayout->addWidget(d);
        }
    }

    // 命令大全（按 group 分区）
    clearLayout(m_commandsLayout);
    m_commandsBox->setVisible(!m_tool.commands.isEmpty());
    QString lastGroup = QStringLiteral("__none__");
    for (const ToolCommand &c : m_tool.commands) {
        if (c.group != lastGroup) {
            lastGroup = c.group;
            if (!c.group.isEmpty()) {
                auto *g = new QLabel(c.group, m_commandsBox);
                g->setObjectName(QStringLiteral("GroupTitle"));
                g->setContentsMargins(0, 6, 0, 0);
                m_commandsLayout->addWidget(g);
            }
        }
        auto *row = new QFrame(m_commandsBox);
        row->setObjectName(QStringLiteral("ExampleRow"));
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(10, 6, 10, 6);
        h->setSpacing(8);
        auto *code = new QLabel(c.cmd, row);
        code->setObjectName(QStringLiteral("CodeText"));
        code->setTextInteractionFlags(Qt::TextSelectableByMouse);
        code->setWordWrap(true);
        auto *copy = new QPushButton(QStringLiteral("复制"), row);
        copy->setObjectName(QStringLiteral("GhostButton"));
        const QString cmd = c.cmd;
        connect(copy, &QPushButton::clicked, this, [cmd]() {
            QApplication::clipboard()->setText(cmd);
        });
        h->addWidget(code, 1);
        h->addWidget(copy, 0, Qt::AlignTop);
        m_commandsLayout->addWidget(row);
        if (!c.desc.isEmpty()) {
            auto *d = new QLabel(QStringLiteral("　└ %1").arg(c.desc), m_commandsBox);
            d->setObjectName(QStringLiteral("Muted"));
            d->setWordWrap(true);
            m_commandsLayout->addWidget(d);
        }
    }

    // 提示与技巧
    clearLayout(m_tipsLayout);
    m_tipsBox->setVisible(!m_tool.tips.isEmpty());
    for (const QString &tip : m_tool.tips) {
        auto *l = new QLabel(QStringLiteral("• %1").arg(tip), m_tipsBox);
        l->setObjectName(QStringLiteral("BodyText"));
        l->setWordWrap(true);
        m_tipsLayout->addWidget(l);
    }

    // 依赖
    clearLayout(m_depsLayout);
    m_depsBox->setVisible(!m_tool.deps.isEmpty());
    for (const QString &dep : m_tool.deps) {
        bool ok = false;
        QString label;
        if (dep == QStringLiteral("java")) {
            ok = m_env.javaOk;
            label = QStringLiteral("Java 运行时");
        } else if (dep == QStringLiteral("python")) {
            ok = m_env.pythonOk;
            label = QStringLiteral("Python 运行时");
        } else if (dep == QStringLiteral("npcap")) {
            ok = m_env.npcapOk;
            label = QStringLiteral("Npcap / WinPcap 驱动");
        } else {
            label = dep;
            ok = true;
        }
        auto *l = new QLabel(QStringLiteral("%1  %2")
                                 .arg(ok ? QStringLiteral("✓") : QStringLiteral("✗"), label),
                             m_depsBox);
        l->setObjectName(ok ? QStringLiteral("DepOk") : QStringLiteral("DepMissing"));
        m_depsLayout->addWidget(l);
    }
}

void ToolDetailPage::setRunning(bool running)
{
    m_primaryBtn->setEnabled(!running && m_tool.ok());
    m_stopBtn->setEnabled(running);
    m_console->setStdinEnabled(running);
}

void ToolDetailPage::appendConsole(const QString &text)
{
    m_console->appendText(text);
}

void ToolDetailPage::clearConsole()
{
    m_console->clearOutput();
}
