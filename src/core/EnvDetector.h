#pragma once

#include <QFutureWatcher>
#include <QString>
#include <QObject>

// 环境依赖检测结果
struct EnvStatus {
    QString javaPath;
    QString pythonPath;
    bool javaOk = false;
    bool pythonOk = false;
    bool npcapOk = false;
};

Q_DECLARE_METATYPE(EnvStatus)

// 异步探测本机依赖：Java / Python / Npcap
class EnvDetector : public QObject
{
    Q_OBJECT
public:
    explicit EnvDetector(QObject *parent = nullptr);

    void detectAsync();
    EnvStatus status() const { return m_status; }

    static EnvStatus detectNow();

signals:
    void detected(const EnvStatus &status);

private:
    EnvStatus m_status;
};
