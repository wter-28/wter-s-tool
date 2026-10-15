#include "ToolManagerDialog.h"
#include "ToolEditorDialog.h"

#include "core/ToolRegistry.h"

#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString stateText(ToolState s)
{
    switch (s) {
    case ToolState::Ok:
    case ToolState::ExternalOk:
        return QStringLiteral("可用");
    case ToolState::NeedCompile:
        return QStringLiteral("需编译");
    case ToolState::ExternalMissing:
    case ToolState::Missing:
    default:
        return QStringLiteral("缺失");
    }
}

} // namespace

ToolManagerDialog::ToolManagerDialog(ToolRegistry *registry, QWidget *parent)
    : QDialog(parent)
    , m_registry(registry)
{
    setWindowTitle(QStringLiteral("工具管理"));
    resize(720, 520);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto *hint = new QLabel(QStringLiteral("双击一行可直接编辑；自定义工具可删除，内置工具「删除」表示恢复默认。"), this);
    hint->setObjectName(QStringLiteral("Muted"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("分类"),
                                        QStringLiteral("来源"), QStringLiteral("状态")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 220);
    m_table->setColumnWidth(1, 150);
    m_table->setColumnWidth(2, 130);
    root->addWidget(m_table, 1);

    auto *btnRow = new QHBoxLayout();
    auto *addBtn = new QPushButton(QStringLiteral("添加工具"), this);
    addBtn->setObjectName(QStringLiteral("PrimaryButton"));
    m_editBtn = new QPushButton(QStringLiteral("编辑"), this);
    m_deleteBtn = new QPushButton(QStringLiteral("删除 / 恢复默认"), this);
    m_hideBtn = new QPushButton(QStringLiteral("隐藏 / 显示"), this);
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setObjectName(QStringLiteral("GhostButton"));
    btnRow->addWidget(addBtn);
    btnRow->addWidget(m_editBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addWidget(m_hideBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    connect(addBtn, &QPushButton::clicked, this, &ToolManagerDialog::onAdd);
    connect(m_editBtn, &QPushButton::clicked, this, &ToolManagerDialog::onEdit);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ToolManagerDialog::onDelete);
    connect(m_hideBtn, &QPushButton::clicked, this, &ToolManagerDialog::onToggleHidden);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ToolManagerDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *) { onEdit(); });

    refresh();
}

void ToolManagerDialog::refresh()
{
    QHash<QString, QString> catNames;
    for (const CategoryInfo &c : m_registry->categories())
        catNames.insert(c.id, c.name.isEmpty() ? c.id : c.name);

    const QList<ToolInfo> tools = m_registry->allTools();
    m_table->setRowCount(tools.size());

    for (int i = 0; i < tools.size(); ++i) {
        const ToolInfo &t = tools.at(i);

        auto *nameItem = new QTableWidgetItem(t.name);
        nameItem->setData(Qt::UserRole, t.id);
        m_table->setItem(i, 0, nameItem);

        m_table->setItem(i, 1, new QTableWidgetItem(catNames.value(t.categoryId, t.categoryId)));

        QString source = QStringLiteral("内置");
        if (t.custom)
            source = QStringLiteral("自定义");
        else if (t.overridden)
            source = QStringLiteral("内置 · 已修改");
        if (t.hidden)
            source += QStringLiteral("（已隐藏）");
        m_table->setItem(i, 2, new QTableWidgetItem(source));

        m_table->setItem(i, 3, new QTableWidgetItem(stateText(t.state)));
    }

    updateButtons();
}

QString ToolManagerDialog::selectedId() const
{
    const int row = m_table->currentRow();
    if (row < 0)
        return QString();
    QTableWidgetItem *item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void ToolManagerDialog::onSelectionChanged()
{
    updateButtons();
}

void ToolManagerDialog::updateButtons()
{
    const QString id = selectedId();
    const ToolInfo *tool = id.isEmpty() ? nullptr : m_registry->findById(id);
    const bool has = tool != nullptr;
    m_editBtn->setEnabled(has);
    m_deleteBtn->setEnabled(has);
    m_hideBtn->setEnabled(has);
    if (has)
        m_hideBtn->setText(tool->hidden ? QStringLiteral("取消隐藏") : QStringLiteral("隐藏"));
}

void ToolManagerDialog::onAdd()
{
    ToolEditorDialog dlg(m_registry->categories(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString err;
    if (!m_registry->upsertUserTool(dlg.tool(), &err)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), err);
        return;
    }
    refresh();
}

void ToolManagerDialog::onEdit()
{
    const QString id = selectedId();
    const ToolInfo *tool = id.isEmpty() ? nullptr : m_registry->findById(id);
    if (!tool)
        return;

    ToolEditorDialog dlg(m_registry->categories(), this);
    dlg.setTool(*tool);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString err;
    if (!m_registry->upsertUserTool(dlg.tool(), &err)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), err);
        return;
    }
    refresh();
}

void ToolManagerDialog::onDelete()
{
    const QString id = selectedId();
    const ToolInfo *tool = id.isEmpty() ? nullptr : m_registry->findById(id);
    if (!tool)
        return;

    const QString text = m_registry->isBuiltinId(id)
                             ? QStringLiteral("将移除对「%1」的自定义修改，恢复内置默认？").arg(tool->name)
                             : QStringLiteral("确定删除自定义工具「%1」？").arg(tool->name);
    if (QMessageBox::question(this, QStringLiteral("确认"), text) != QMessageBox::Yes)
        return;

    QString err;
    if (!m_registry->removeUserTool(id, &err)) {
        QMessageBox::warning(this, QStringLiteral("操作失败"), err);
        return;
    }
    refresh();
}

void ToolManagerDialog::onToggleHidden()
{
    const QString id = selectedId();
    const ToolInfo *tool = id.isEmpty() ? nullptr : m_registry->findById(id);
    if (!tool)
        return;

    QString err;
    if (!m_registry->setUserHidden(id, !tool->hidden, &err)) {
        QMessageBox::warning(this, QStringLiteral("操作失败"), err);
        return;
    }
    refresh();
}