#pragma once

#include <QWidget>

#include "core/EnvDetector.h"
#include "core/ToolInfo.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class ConsoleWidget;

// 工具详情页：中文讲解 + 命令示例 + 依赖状态 + 启动/运行 + 内嵌控制台
class ToolDetailPage : public QWidget
{
    Q_OBJECT
public:
    explicit ToolDetailPage(QWidget *parent = nullptr);

    void loadTool(const ToolInfo &tool, const EnvStatus &env);
    void setEnv(const EnvStatus &env);
    void setRunning(bool running);
    void appendConsole(const QString &text);
    void clearConsole();

signals:
    void backRequested();
    void launchRequested(const QString &id);
    void runRequested(const QString &id, const QStringList &args);
    void prepareInputsRequested(const QString &id);
    void stopRequested();
    void stdinSubmitted(const QString &text);
    void openFolderRequested(const QString &path);
    void openUrlRequested(const QString &url);

private:
    void buildUi();
    void rebuildDynamic();

    ToolInfo m_tool;
    EnvStatus m_env;
    QString m_id;

    QLabel *m_icon = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_badges = nullptr;
    QLabel *m_desc = nullptr;
    QLabel *m_usage = nullptr;
    QLabel *m_path = nullptr;
    QLabel *m_note = nullptr;

    QVBoxLayout *m_examplesLayout = nullptr;
    QVBoxLayout *m_commandsLayout = nullptr;
    QVBoxLayout *m_tipsLayout = nullptr;
    QVBoxLayout *m_depsLayout = nullptr;

    QWidget *m_examplesBox = nullptr;
    QWidget *m_commandsBox = nullptr;
    QWidget *m_tipsBox = nullptr;
    QWidget *m_depsBox = nullptr;

    QLineEdit *m_args = nullptr;
    QPushButton *m_primaryBtn = nullptr;
    QPushButton *m_inputsBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_folderBtn = nullptr;
    QPushButton *m_websiteBtn = nullptr;

    QWidget *m_runSection = nullptr;
    ConsoleWidget *m_console = nullptr;
};
