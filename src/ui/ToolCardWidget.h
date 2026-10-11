#pragma once

#include <QFrame>

#include "core/ToolInfo.h"

class QLabel;

// 单个工具卡片
class ToolCardWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ToolCardWidget(QWidget *parent = nullptr);

    void setTool(const ToolInfo &tool);
    QString toolId() const { return m_id; }

signals:
    void activated(const QString &id);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_id;
    QLabel *m_icon = nullptr;
    QLabel *m_name = nullptr;
    QLabel *m_category = nullptr;
    QLabel *m_desc = nullptr;
    QLabel *m_badge = nullptr;
};
