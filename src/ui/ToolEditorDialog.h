#pragma once

#include <QDialog>
#include <QList>
#include <QString>

#include "core/ToolInfo.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class DropZone;

// 添加 / 编辑用户自定义工具的表单对话框
class ToolEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ToolEditorDialog(const QList<CategoryInfo> &categories, QWidget *parent = nullptr);

    // 载入已有工具用于编辑（保留 id 与未在表单中展示的字段）
    void setTool(const ToolInfo &tool);

    // 由拖入 / 选中的文件预填（推断类型、启动方式与名称）
    void presetFromFile(const QString &path);

    // 读取表单结果
    ToolInfo tool() const;

private slots:
    void onTargetChanged();
    void onBrowseWorkdir();
    void onAccept();

private:
    void buildUi();
    void applyTypeInference(const QString &path);
    static QString makeId(const QString &name);

    QList<CategoryInfo> m_categories;
    ToolInfo m_base;          // 编辑时的原始工具（用于保留未展示字段）
    QString m_resultId;       // 编辑时沿用原 id；新建时为空，保存时生成

    QLineEdit *m_name = nullptr;
    QComboBox *m_category = nullptr;
    QComboBox *m_type = nullptr;
    QComboBox *m_launchKind = nullptr;
    DropZone *m_target = nullptr;
    QLineEdit *m_workdir = nullptr;
    QLineEdit *m_argsPlaceholder = nullptr;
    QLineEdit *m_defaultArgs = nullptr;
    QCheckBox *m_needAdmin = nullptr;
    QLineEdit *m_website = nullptr;
    QPlainTextEdit *m_description = nullptr;
    QPlainTextEdit *m_usage = nullptr;
};