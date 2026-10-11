#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

#include "core/ToolInfo.h"

class QGridLayout;
class QLabel;
class QScrollArea;
class ToolCardWidget;

// 工具卡片网格（支持分类过滤 + 关键字搜索 + 自适应列数）
class ToolGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolGridWidget(QWidget *parent = nullptr);

    void setTools(const QList<ToolInfo> &tools);
    void setCategoryFilter(const QString &categoryId);
    void setTextFilter(const QString &text);

signals:
    void toolActivated(const QString &id);
    void countChanged(int visible, int total);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void rebuildCards();
    void applyFilter();
    void relayout();
    int columnCount() const;

    QList<ToolInfo> m_tools;
    QHash<QString, ToolCardWidget *> m_cards;
    QList<ToolCardWidget *> m_visible;

    QString m_category = QStringLiteral("all");
    QString m_text;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QGridLayout *m_grid = nullptr;
    QLabel *m_empty = nullptr;
};
