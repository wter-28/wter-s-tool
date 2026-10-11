#include "ToolLauncher.h"
#include "AppSettings.h"
#include "PathResolver.h"
#include "util/Log.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

ToolLauncher::ToolLauncher(QObject *parent)
    : QObject(parent)
{
}

static QString javaExecutable()
{
    const QString configured = AppSettings::instance().javaPath();
    if (!configured.isEmpty() && QFileInfo::exists(configured))
        return configured;
    QString j = QStandardPaths::findExecutable(QStringLiteral("javaw"));
    if (j.isEmpty())
        j = QStandardPaths::findExecutable(QStringLiteral("java"));
    return j.isEmpty() ? QStringLiteral("java") : j;
}

static QString pythonExecutable()
{
    const QString configured = AppSettings::instance().pythonPath();
    if (!configured.isEmpty() && QFileInfo::exists(configured))
        return configured;
    QString p = QStandardPaths::findExecutable(QStringLiteral("python"));
    if (p.isEmpty())
        p = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (p.isEmpty())
        p = QStandardPaths::findExecutable(QStringLiteral("py"));
    return p.isEmpty() ? QStringLiteral("python") : p;
}

bool ToolLauncher::buildCommand(const ToolInfo &tool, const QStringList &extraArgs,
                                QString &program, QStringList &args)
{
    args.clear();
    switch (tool.type) {
    case ToolType::Exe:
    case ToolType::Folder:
        program = tool.resolvedPath;
        break;
    case ToolType::Bat:
        program = QStringLiteral("cmd.exe");
        args << QStringLiteral("/c") << tool.resolvedPath;
        break;
    case ToolType::Jar:
        program = javaExecutable();
        args << QStringLiteral("-jar") << tool.resolvedPath;
        break;
    case ToolType::Python:
        program = pythonExecutable();
        args << tool.resolvedPath;
        break;
    case ToolType::Html:
    case ToolType::Unknown:
    default:
        return false;
    }

    args += tool.defaultArgs;
    args += extraArgs;
    return true;
}

QString ToolLauncher::workingDirectory(const ToolInfo &tool)
{
    if (!tool.workdir.isEmpty()) {
        const QString wd = PathResolver::join(AppSettings::instance().toolsRoot(), tool.workdir);
        if (PathResolver::exists(wd))
            return wd;
    }
    return PathResolver::parentDir(tool.resolvedPath);
}

#ifdef Q_OS_WIN
static bool shellExecute(const QString &program, const QStringList &args,
                         const QString &workdir, bool asAdmin, QString *err)
{
    QString argLine;
    for (const QString &a : args) {
        if (!argLine.isEmpty())
            argLine += QLatin1Char(' ');
        // 简单引号包裹（参数不含需要转义的复杂字符时足够）
        if (a.contains(QLatin1Char(' ')) || a.contains(QLatin1Char('"')))
            argLine += QLatin1Char('"') + QString(a).replace(QLatin1Char('"'), QStringLiteral("\\\"")) + QLatin1Char('"');
        else
            argLine += a;
    }

    const std::wstring wProg = program.toStdWString();
    const std::wstring wArgs = argLine.toStdWString();
    const std::wstring wDir = workdir.toStdWString();

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = asAdmin ? L"runas" : L"open";
    sei.lpFile = wProg.c_str();
    sei.lpParameters = wArgs.empty() ? nullptr : wArgs.c_str();
    sei.lpDirectory = wDir.empty() ? nullptr : wDir.c_str();
    sei.nShow = SW_NORMAL;

    if (!ShellExecuteExW(&sei)) {
        if (err)
            *err = QStringLiteral("启动失败（错误码 %1）").arg(GetLastError());
        return false;
    }
    if (sei.hProcess)
        CloseHandle(sei.hProcess);
    return true;
}
#endif

bool ToolLauncher::launch(const ToolInfo &tool, QString *err)
{
    if (!tool.ok()) {
        if (err)
            *err = QStringLiteral("工具不可用（%1）").arg(tool.resolvedPath);
        return false;
    }

    // HTML：交给系统默认程序（浏览器）打开
    if (tool.type == ToolType::Html || tool.launchKind == LaunchKind::OpenUrl) {
        openUrl(QUrl::fromLocalFile(tool.resolvedPath).toString());
        return true;
    }

    // 仅打开目录
    if (tool.type == ToolType::Folder || tool.launchKind == LaunchKind::Folder) {
        openFolder(tool.resolvedPath);
        return true;
    }

    QString program;
    QStringList args;
    if (!buildCommand(tool, {}, program, args)) {
        if (err)
            *err = QStringLiteral("无法识别的工具类型");
        return false;
    }

    const QString wd = workingDirectory(tool);

    // 需要管理员权限：走 runas
    if (tool.needAdmin) {
#ifdef Q_OS_WIN
        return shellExecute(program, args, wd, true, err);
#else
        // 非 Windows 平台忽略提权
#endif
    }

    qint64 pid = 0;
    const bool ok = QProcess::startDetached(program, args, wd, &pid);
    if (!ok && err)
        *err = QStringLiteral("启动失败：%1").arg(program);
    else
        Log::info(QStringLiteral("已启动：%1 %2").arg(program, args.join(QLatin1Char(' '))));
    return ok;
}

bool ToolLauncher::startStreaming(const ToolInfo &tool, const QStringList &extraArgs, QString *err)
{
    if (isRunning()) {
        if (err)
            *err = QStringLiteral("已有任务正在运行，请先停止");
        return false;
    }
    if (!tool.ok()) {
        if (err)
            *err = QStringLiteral("工具不可用（%1）").arg(tool.resolvedPath);
        return false;
    }

    QString program;
    QStringList args;
    if (!buildCommand(tool, extraArgs, program, args)) {
        if (err)
            *err = QStringLiteral("该工具不支持命令行运行");
        return false;
    }

    if (!m_proc) {
        m_proc = new QProcess(this);
        m_proc->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_proc, &QProcess::readyReadStandardOutput, this, [this]() {
            const QString out = QString::fromLocal8Bit(m_proc->readAllStandardOutput());
            if (!out.isEmpty())
                emit outputReady(out);
        });
        connect(m_proc, &QProcess::readyReadStandardError, this, [this]() {
            const QString out = QString::fromLocal8Bit(m_proc->readAllStandardError());
            if (!out.isEmpty())
                emit outputReady(out);
        });
        connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [this](int code, QProcess::ExitStatus status) {
                    emit finished(code, status == QProcess::CrashExit);
                });
        connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            emit outputReady(QStringLiteral("\n[进程错误] %1\n").arg(m_proc->errorString()));
        });
    }

    const QString wd = workingDirectory(tool);
    m_proc->setWorkingDirectory(wd);

    const QString cmdLine = program + QLatin1Char(' ') + args.join(QLatin1Char(' '));
    emit outputReady(QStringLiteral("> %1\n\n").arg(cmdLine));
    emit started(cmdLine);

    m_proc->start(program, args);
    if (!m_proc->waitForStarted(5000)) {
        if (err)
            *err = QStringLiteral("进程启动超时：%1").arg(program);
        return false;
    }
    return true;
}

void ToolLauncher::writeStdin(const QString &text)
{
    if (m_proc && m_proc->state() == QProcess::Running) {
        m_proc->write(text.toLocal8Bit());
        if (!text.endsWith(QLatin1Char('\n')))
            m_proc->write("\n");
    }
}

void ToolLauncher::stop()
{
    if (!m_proc || m_proc->state() == QProcess::NotRunning)
        return;
#ifdef Q_OS_WIN
    const qint64 pid = m_proc->processId();
    if (pid > 0) {
        QProcess::execute(QStringLiteral("taskkill"),
                          {QStringLiteral("/T"), QStringLiteral("/F"),
                           QStringLiteral("/PID"), QString::number(pid)});
    }
#else
    m_proc->terminate();
#endif
}

bool ToolLauncher::isRunning() const
{
    return m_proc && m_proc->state() != QProcess::NotRunning;
}

void ToolLauncher::openFolder(const QString &path)
{
    if (path.isEmpty())
        return;
#ifdef Q_OS_WIN
    const QFileInfo fi(path);
    if (fi.isFile()) {
        // explorer 的 /select 需要反斜杠路径
        QString native = QDir::toNativeSeparators(fi.absoluteFilePath());
        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                {QStringLiteral("/select,") + native});
    } else {
        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                {QDir::toNativeSeparators(path)});
    }
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}

void ToolLauncher::openUrl(const QString &url)
{
    if (url.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl(url));
}
