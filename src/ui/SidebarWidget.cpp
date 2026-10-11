#include "SidebarWidget.h"

#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setFixedWidth(190);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 10, 8, 10);
    root->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("工具分类"), this);
    title->setObjectName(QStringLiteral("SidebarTitle"));
    root->addWidget(title);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("CategoryList"));
    m_list->setFrameShape(QFrame::NoFrame);
    root->addWidget(m_list, 1);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = m_list->item(row);
        if (!item)
            return;
        emit categoryChanged(item->data(Qt::UserRole).toString());
    });
}

void SidebarWidget::setCategories(const QList<CategoryInfo> &categories,
                                  const QHash<QString, int> &counts, int totalCount)
{
    const QString keep = currentCategory().isEmpty() ? QStringLiteral("all") : currentCategory();

    m_list->blockSignals(true);
    m_list->clear();

    auto addItem = [this](const QString &id, const QString &name, int count) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  (%2)").arg(name).arg(count));
        item->setData(Qt::UserRole, id);
        item->setToolTip(name);
        m_list->addItem(item);
    };

    addItem(QStringLiteral("all"), QStringLiteral("全部工具"), totalCount);
    for (const CategoryInfo &c : categories)
        addItem(c.id, c.name, counts.value(c.id, 0));

    // 恢复选中项
    int target = 0;
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toString() == keep) {
            target = i;
            break;
        }
    }
    m_list->setCurrentRow(target);
    m_list->blockSignals(false);
    emit categoryChanged(m_list->item(target)->data(Qt::UserRole).toString());
}

QString SidebarWidget::currentCategory() const
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item)
        return QStringLiteral("all");
    return item->data(Qt::UserRole).toString();
}

void SidebarWidget::selectAll()
{
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}
