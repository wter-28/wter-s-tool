#include "ToolEditorDialog.h"
#include "DragDropInputDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

namespace {

int indexOfData(QComboBox *box, const QVariant &data)
{
    for (int i = 0; i < box->count(); ++i) {
        if (box->itemData(i) == data)
            return i;
    }
    return -1;
}

} // namespace

ToolEditorDialog::ToolEditorDialog(const QList<CategoryInfo> &categories, QWidget *parent)
    : QDialog(parent)
    , m_categories(categories)
{
    setWindowTitle(QStringLiteral("添加 / 编辑工具"));
    setMinimumWidth(560);
    buildUi();
}

void ToolEditorDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);

    m_name = new QLineEdit(this);
    form->addRow(QStringLiteral("名称"), m_name);

    m_category = new QComboBox(this);
    for (const CategoryInfo &c : m_categories) {
        const QString label = c.name.isEmpty() ? c.id : c.name;
        m_category->addItem(label, c.id);
    }
    if (m_category->count() == 0)
        m_category->addItem(QStringLiteral("未分类"), QStringLiteral("mystery"));
    form->addRow(QStringLiteral("分类"), m_category);

    m_type = new QComboBox(this);
    m_type->addItem(QStringLiteral("可执行文件 (exe)"), static_cast<int>(ToolType::Exe));
    m_type->addItem(QStringLiteral("Python 脚本"), static_cast<int>(ToolType::Python));
    m_type->addItem(QStringLiteral("批处理 (bat)"), static_cast<int>(ToolType::Bat));
    m_type->addItem(QStringLiteral("Java 程序 (jar)"), static_cast<int>(ToolType::Jar));
    m_type->addItem(QStringLiteral("网页 (html)"), static_cast<int>(ToolType::Html));
    m_type->addItem(QStringLiteral("仅目录"), static_cast<int>(ToolType::Folder));
    form->addRow(QStringLiteral("类型"), m_type);

    m_launchKind = new QComboBox(this);
    m_launchKind->addItem(QStringLiteral("图形界面（分离启动）"), static_cast<int>(LaunchKind::Gui));
    m_launchKind->addItem(QStringLiteral("命令行（内置运行器）"), static_cast<int>(LaunchKind::Cli));
    m_launchKind->addItem(QStringLiteral("用浏览器打开"), static_cast<int>(LaunchKind::OpenUrl));
    m_launchKind->addItem(QStringLiteral("仅打开目录"), static_cast<int>(LaunchKind::Folder));
    form->addRow(QStringLiteral("启动方式"), m_launchKind);

    m_target = new DropZone(this);
    m_target->setFilter(QStringLiteral("程序 / 脚本 (*.exe *.py *.bat *.cmd *.jar *.html *.htm);;所有文件 (*.*)"));
    m_target->setOnChanged([this](const QString &) { onTargetChanged(); });
    form->addRow(QStringLiteral("可执行文件"), m_target);

    auto *wdRow = new QWidget(this);
    auto *wdLayout = new QHBoxLayout(wdRow);
    wdLayout->setContentsMargins(0, 0, 0, 0);
    wdLayout->setSpacing(6);
    m_workdir = new QLineEdit(wdRow);
    m_workdir->setPlaceholderText(QStringLiteral("留空则使用程序所在目录"));
    auto *wdBtn = new QPushButton(QStringLiteral("浏览…"), wdRow);
    wdBtn->setObjectName(QStringLiteral("GhostButton"));
    wdLayout->addWidget(m_workdir, 1);
    wdLayout->addWidget(wdBtn);
    connect(wdBtn, &QPushButton::clicked, this, &ToolEditorDialog::onBrowseWorkdir);
    form->addRow(QStringLiteral("工作目录"), wdRow);

    m_argsPlaceholder = new QLineEdit(this);
    m_argsPlaceholder->setPlaceholderText(QStringLiteral("例如 -d example.com"));
    form->addRow(QStringLiteral("参数提示"), m_argsPlaceholder);

    m_defaultArgs = new QLineEdit(this);
    m_defaultArgs->setPlaceholderText(QStringLiteral("每次运行都会带上，空格分隔"));
    form->addRow(QStringLiteral("默认参数"), m_defaultArgs);

    m_needAdmin = new QCheckBox(QStringLiteral("需要管理员权限启动"), this);
    form->addRow(QString(), m_needAdmin);

    m_website = new QLineEdit(this);
    m_website->setPlaceholderText(QStringLiteral("https://..."));
    form->addRow(QStringLiteral("官网"), m_website);

    m_description = new QPlainTextEdit(this);
    m_description->setFixedHeight(64);
    form->addRow(QStringLiteral("简介"), m_description);

    m_usage = new QPlainTextEdit(this);
    m_usage->setFixedHeight(84);
    form->addRow(QStringLiteral("使用说明"), m_usage);

    root->addLayout(form);
    root->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    cancelBtn->setObjectName(QStringLiteral("GhostButton"));
    auto *saveBtn = new QPushButton(QStringLiteral("保存"), this);
    saveBtn->setObjectName(QStringLiteral("PrimaryButton"));
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    root->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &ToolEditorDialog::onAccept);
}

void ToolEditorDialog::setTool(const ToolInfo &tool)
{
    m_base = tool;
    m_resultId = tool.id;

    m_name->setText(tool.name);
    const int ci = indexOfData(m_category, tool.categoryId);
    if (ci >= 0)
        m_category->setCurrentIndex(ci);
    const int ti = indexOfData(m_type, static_cast<int>(tool.type));
    if (ti >= 0)
        m_type->setCurrentIndex(ti);
    const int li = indexOfData(m_launchKind, static_cast<int>(tool.launchKind));
    if (li >= 0)
        m_launchKind->setCurrentIndex(li);

    if (!tool.execTarget.isEmpty())
        m_target->setSource(tool.execTarget);
    m_workdir->setText(tool.workdir);
    m_argsPlaceholder->setText(tool.argsPlaceholder);
    m_defaultArgs->setText(tool.defaultArgs.join(QLatin1Char(' ')));
    m_needAdmin->setChecked(tool.needAdmin);
    m_website->setText(tool.website);
    m_description->setPlainText(tool.description);
    m_usage->setPlainText(tool.usage);
}

void ToolEditorDialog::presetFromFile(const QString &path)
{
    m_target->setSource(path);
    onTargetChanged();
}

void ToolEditorDialog::onTargetChanged()
{
    const QString path = m_target->source();
    if (path.isEmpty())
        return;

    if (m_workdir->text().trimmed().isEmpty())
        m_workdir->setText(QDir::toNativeSeparators(QFileInfo(path).absolutePath()));
    if (m_name->text().trimmed().isEmpty())
        m_name->setText(QFileInfo(path).completeBaseName());

    applyTypeInference(path);
}

void ToolEditorDialog::applyTypeInference(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();

    ToolType type = ToolType::Exe;
    LaunchKind kind = LaunchKind::Gui;
    if (suffix == QStringLiteral("py")) {
        type = ToolType::Python;
        kind = LaunchKind::Cli;
    } else if (suffix == QStringLiteral("bat") || suffix == QStringLiteral("cmd")) {
        type = ToolType::Bat;
        kind = LaunchKind::Cli;
    } else if (suffix == QStringLiteral("jar")) {
        type = ToolType::Jar;
        kind = LaunchKind::Cli;
    } else if (suffix == QStringLiteral("html") || suffix == QStringLiteral("htm")) {
        type = ToolType::Html;
        kind = LaunchKind::OpenUrl;
    }

    const int ti = indexOfData(m_type, static_cast<int>(type));
    if (ti >= 0)
        m_type->setCurrentIndex(ti);
    const int li = indexOfData(m_launchKind, static_cast<int>(kind));
    if (li >= 0)
        m_launchKind->setCurrentIndex(li);
}

void ToolEditorDialog::onBrowseWorkdir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择工作目录"));
    if (!dir.isEmpty())
        m_workdir->setText(QDir::toNativeSeparators(dir));
}

QString ToolEditorDialog::makeId(const QString &name)
{
    QString slug;
    for (const QChar &ch : name) {
        if (ch.unicode() < 128 && ch.isLetterOrNumber())
            slug.append(ch.toLower());
        else if (ch == QLatin1Char(' ') || ch == QLatin1Char('-') || ch == QLatin1Char('_'))
            slug.append(QLatin1Char('_'));
    }
    if (slug.isEmpty())
        slug = QStringLiteral("tool");
    return slug + QLatin1Char('_') + QString::number(QDateTime::currentSecsSinceEpoch());
}

void ToolEditorDialog::onAccept()
{
    const QString name = m_name->text().trimmed();
    const QString target = m_target->source().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请填写工具名称。"));
        return;
    }
    if (target.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择可执行文件或脚本。"));
        return;
    }
    if (m_resultId.isEmpty())
        m_resultId = makeId(name);
    accept();
}

ToolInfo ToolEditorDialog::tool() const
{
    ToolInfo t = m_base;   // 保留未在表单中展示的字段
    t.id = m_resultId;
    t.name = m_name->text().trimmed();
    t.categoryId = m_category->currentData().toString();
    t.type = static_cast<ToolType>(m_type->currentData().toInt());
    t.launchKind = static_cast<LaunchKind>(m_launchKind->currentData().toInt());

    const QString target = m_target->source().trimmed();
    const bool abs = QFileInfo(target).isAbsolute();
    t.external = abs;
    t.execTarget = abs ? QDir::toNativeSeparators(target) : QDir::fromNativeSeparators(target);

    t.workdir = m_workdir->text().trimmed();
    t.argsPlaceholder = m_argsPlaceholder->text().trimmed();
    t.defaultArgs = m_defaultArgs->text().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    t.needAdmin = m_needAdmin->isChecked();
    t.website = m_website->text().trimmed();
    t.description = m_description->toPlainText().trimmed();
    t.usage = m_usage->toPlainText().trimmed();
    t.hidden = false;
    t.runnable = (t.type != ToolType::Html
                  && t.launchKind != LaunchKind::Gui
                  && t.launchKind != LaunchKind::OpenUrl
                  && t.launchKind != LaunchKind::Folder);
    return t;
}