#pragma once

#include <QDialog>
#include <QString>

#include "core/ToolInfo.h"

class QPushButton;
class QTableWidget;
class ToolRegistry;

// 工具管理器：列出全部工具，支持添加 / 编辑 / 删除 / 隐藏
class ToolManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ToolManagerDialog(ToolRegistry *registry, QWidget *parent = nullptr);

private slots:
    void refresh();
    void onAdd();
    void onEdit();
    void onDelete();
    void onToggleHidden();
    void onSelectionChanged();

private:
    QString selectedId() const;
    void updateButtons();

    ToolRegistry *m_registry = nullptr;
    QTableWidget *m_table = nullptr;
    QPushButton *m_editBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;
    QPushButton *m_hideBtn = nullptr;
};