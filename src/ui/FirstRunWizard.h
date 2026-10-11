#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

// 首次运行引导 / 设置对话框：配置工具根目录与 Java、Python 路径
class FirstRunWizard : public QDialog
{
    Q_OBJECT
public:
    explicit FirstRunWizard(bool firstRun, QWidget *parent = nullptr);

    QString toolsRoot() const;
    QString javaPath() const;
    QString pythonPath() const;

private:
    void browseInto(QLineEdit *edit, const QString &title);
    void validate();

    bool m_firstRun = true;
    QLineEdit *m_root = nullptr;
    QLineEdit *m_java = nullptr;
    QLineEdit *m_python = nullptr;
    QLabel *m_hint = nullptr;
    QPushButton *m_ok = nullptr;
};
