#include "EnvDetector.h"
#include "AppSettings.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>

EnvDetector::EnvDetector(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<EnvStatus>("EnvStatus");
}

EnvStatus EnvDetector::detectNow()
{
    EnvStatus s;
    const AppSettings &cfg = AppSettings::instance();

    // ---- Java ----
    const QString cfgJava = cfg.javaPath();
    if (!cfgJava.isEmpty() && QFileInfo::exists(cfgJava)) {
        s.javaPath = cfgJava;
    } else {
        QString jp = QStandardPaths::findExecutable(QStringLiteral("java"));
        if (jp.isEmpty())
            jp = QStandardPaths::findExecutable(QStringLiteral("javaw"));
        if (jp.isEmpty()) {
            const QString jhome = qEnvironmentVariable("JAVA_HOME");
            if (!jhome.isEmpty()) {
                const QString cand = jhome + QStringLiteral("/bin/java.exe");
                if (QFileInfo::exists(cand))
                    jp = cand;
            }
        }
        s.javaPath = jp;
    }
    s.javaOk = !s.javaPath.isEmpty();

    // ---- Python ----
    const QString cfgPy = cfg.pythonPath();
    if (!cfgPy.isEmpty() && QFileInfo::exists(cfgPy)) {
        s.pythonPath = cfgPy;
    } else {
        QString pp = QStandardPaths::findExecutable(QStringLiteral("python"));
        if (pp.isEmpty())
            pp = QStandardPaths::findExecutable(QStringLiteral("python3"));
        if (pp.isEmpty())
            pp = QStandardPaths::findExecutable(QStringLiteral("py"));
        s.pythonPath = pp;
    }
    s.pythonOk = !s.pythonPath.isEmpty();

    // ---- Npcap（ksubdomain 发包依赖）----
    const QString sys = qEnvironmentVariable("SystemRoot", QStringLiteral("C:/Windows"));
    s.npcapOk = QFileInfo::exists(sys + QStringLiteral("/System32/wpcap.dll"))
                || QFileInfo::exists(sys + QStringLiteral("/System32/Npcap/wpcap.dll"));

    return s;
}

void EnvDetector::detectAsync()
{
    auto *watcher = new QFutureWatcher<EnvStatus>(this);
    connect(watcher, &QFutureWatcher<EnvStatus>::finished, this, [this, watcher]() {
        m_status = watcher->result();
        watcher->deleteLater();
        emit detected(m_status);
    });
    watcher->setFuture(QtConcurrent::run([]() { return EnvDetector::detectNow(); }));
}
