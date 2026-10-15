#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include "ToolInfo.h"

// 工具启动器：负责 GUI 分离启动、CLI 流式运行、打开目录、打开 URL
class ToolLauncher : public QObject
{
    Q_OBJECT
public:
    explicit ToolLauncher(QObject *parent = nullptr);

    // 分离启动（GUI / bat / jar / python / html / folder）
    // 返回 false 时 err 会带上错误信息
    bool launch(const ToolInfo &tool, QString *err = nullptr);

    // 在内置控制台中流式运行（CLI）
    bool startStreaming(const ToolInfo &tool, const QStringList &extraArgs, QString *err = nullptr);

    void writeStdin(const QString &text);
    void stop();
    bool isRunning() const;

    static void openFolder(const QString &path);
    static void openUrl(const QString &url);

    // 计算工具的实际工作目录（workdir 或可执行文件所在目录）
    static QString workingDirectory(const ToolInfo &tool);

    // 构造实际执行程序与参数（供详情页预览命令）
    static bool buildCommand(const ToolInfo &tool, const QStringList &extraArgs,
                             QString &program, QStringList &args);

    // 从「命令大全/示例」的完整命令行中剥出要传给本工具的参数。
    // 可一键运行时返回 true（args 为参数）；无法识别（如 tshark / UI 步骤）返回 false。
    static bool deriveRunArgs(const ToolInfo &tool, const QString &commandLine, QStringList &args);

signals:
    void outputReady(const QString &text);
    void started(const QString &commandLine);
    void finished(int exitCode, bool crashed);

private:
    QProcess *m_proc = nullptr;
};
