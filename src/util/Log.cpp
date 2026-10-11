#include "Log.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace {
QString g_logPath;

void writeLine(const QString &level, const QString &msg)
{
    const QString line = QStringLiteral("[%1] [%2] %3")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                                  level, msg);
    qDebug().noquote() << line;
    if (g_logPath.isEmpty())
        return;
    QFile f(g_logPath);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);
        ts << line << '\n';
    }
}
} // namespace

namespace Log {

void init()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    g_logPath = dir + QStringLiteral("/ctftoolbox.log");
}

QString logFilePath()
{
    return g_logPath;
}

void info(const QString &msg) { writeLine(QStringLiteral("INFO"), msg); }
void warn(const QString &msg) { writeLine(QStringLiteral("WARN"), msg); }
void error(const QString &msg) { writeLine(QStringLiteral("ERROR"), msg); }

} // namespace Log
