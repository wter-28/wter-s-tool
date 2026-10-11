#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

#include "core/ToolInfo.h"

class QListWidget;

// 左侧分类导航
class SidebarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SidebarWidget(QWidget *parent = nullptr);

    void setCategories(const QList<CategoryInfo> &categories,
                       const QHash<QString, int> &counts, int totalCount);
    QString currentCategory() const;
    void selectAll();

signals:
    void categoryChanged(const QString &categoryId);

private:
    QListWidget *m_list = nullptr;
};
