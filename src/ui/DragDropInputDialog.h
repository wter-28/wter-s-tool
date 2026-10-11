#pragma once

#include <QDialog>
#include <QFrame>
#include <QList>
#include <QString>

#include <functional>

#include "core/ToolInfo.h"

class QLabel;

// 拖拽 / 点击选择文件的放置区（不使用信号，改用回调通知，避免额外 moc）
class DropZone : public QFrame
{
public:
    explicit DropZone(QWidget *parent = nullptr);

    void setFilter(const QString &filter);   // 文件选择对话框的过滤器
    void setSource(const QString &path);     // 设置已选文件路径
    QString source() const { return m_source; }
    void setOnChanged(std::function<void(const QString &)> cb) { m_onChanged = std::move(cb); }

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    void choose();
    void setFromUrl(const QUrl &url);
    void updateText();

    QString m_filter;
    QString m_source;
    bool m_hover = false;
    std::function<void(const QString &)> m_onChanged;
};

// 前置文件准备对话框：列出工具所需输入，支持拖拽 / 选择，确认后复制到工具工作目录
class DragDropInputDialog : public QDialog
{
    Q_OBJECT
public:
    DragDropInputDialog(const ToolInfo &tool, const QString &workdir, QWidget *parent = nullptr);

    // 把用户选择的文件复制到目标位置，成功返回 true，失败时 err 带原因
    bool applyInputs(QString *err = nullptr);

    // 便捷入口：工具无前置文件直接返回 true；否则弹窗，用户确认且复制成功返回 true
    static bool prepare(const ToolInfo &tool, const QString &workdir, QWidget *parent, QString *err = nullptr);

private slots:
    void tryAccept();

private:
    struct Row {
        ToolInput input;
        QString destPath;      // 解析后的绝对目标路径
        DropZone *zone = nullptr;
        QLabel *status = nullptr;
    };

    QList<Row> m_rows;
    QString m_workdir;
};
