#pragma once

#include <QWidget>

class QLineEdit;
class QPlainTextEdit;

// 内置控制台：显示命令行工具的实时输出，并支持向进程写入 stdin
class ConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleWidget(QWidget *parent = nullptr);

    void appendText(const QString &text);
    void clearOutput();
    void setStdinEnabled(bool enabled);

signals:
    void stdinSubmitted(const QString &text);

private:
    QPlainTextEdit *m_output = nullptr;
    QLineEdit *m_input = nullptr;
};
