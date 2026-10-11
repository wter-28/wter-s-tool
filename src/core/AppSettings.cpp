#include "AppSettings.h"
#include "PathResolver.h"

#include <QCoreApplication>
#include <QFileInfo>

AppSettings &AppSettings::instance()
{
    static AppSettings s;
    return s;
}

AppSettings::AppSettings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("CTFToolbox"), QStringLiteral("CTFToolbox"))
{
}

QString AppSettings::portableToolsRoot()
{
    const QString dir = QCoreApplication::applicationDirPath()
                        + QLatin1Char('/') + QStringLiteral("tools");
    if (QFileInfo(dir).isDir())
        return PathResolver::normalize(dir);
    return QString();
}

QString AppSettings::toolsRoot() const
{
    // 便携包优先：exe 同级存在 tools 目录时免配置直接使用
    const QString portable = portableToolsRoot();
    if (!portable.isEmpty())
        return portable;
    return PathResolver::normalize(m_settings.value(QStringLiteral("toolsRoot")).toString());
}

void AppSettings::setToolsRoot(const QString &path)
{
    const QString clean = PathResolver::normalize(path);
    if (clean == toolsRoot())
        return;
    m_settings.setValue(QStringLiteral("toolsRoot"), clean);
    m_settings.sync();
    emit toolsRootChanged(clean);
}

QString AppSettings::javaPath() const
{
    return m_settings.value(QStringLiteral("javaPath")).toString();
}

void AppSettings::setJavaPath(const QString &path)
{
    m_settings.setValue(QStringLiteral("javaPath"), path);
    m_settings.sync();
}

QString AppSettings::pythonPath() const
{
    return m_settings.value(QStringLiteral("pythonPath")).toString();
}

void AppSettings::setPythonPath(const QString &path)
{
    m_settings.setValue(QStringLiteral("pythonPath"), path);
    m_settings.sync();
}

bool AppSettings::firstRunDone() const
{
    return m_settings.value(QStringLiteral("firstRunDone"), false).toBool();
}

void AppSettings::setFirstRunDone(bool done)
{
    m_settings.setValue(QStringLiteral("firstRunDone"), done);
    m_settings.sync();
}

QString AppSettings::lastToolId() const
{
    return m_settings.value(QStringLiteral("lastToolId")).toString();
}

void AppSettings::setLastToolId(const QString &id)
{
    m_settings.setValue(QStringLiteral("lastToolId"), id);
    m_settings.sync();
}
