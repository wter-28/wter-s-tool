#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

// 应用配置（存于 %APPDATA%/CTFToolbox/CTFToolbox.ini，便于绿色迁移）
class AppSettings : public QObject
{
    Q_OBJECT
public:
    static AppSettings &instance();

    // 便携模式：程序目录下若存在 tools 子目录，则以其为工具根目录（免配置）
    static QString portableToolsRoot();

    QString toolsRoot() const;
    void setToolsRoot(const QString &path);

    QString javaPath() const;
    void setJavaPath(const QString &path);

    QString pythonPath() const;
    void setPythonPath(const QString &path);

    bool firstRunDone() const;
    void setFirstRunDone(bool done);

    QString lastToolId() const;
    void setLastToolId(const QString &id);

signals:
    void toolsRootChanged(const QString &root);

private:
    AppSettings();
    QSettings m_settings;
};
