#include <QApplication>
#include <QFile>
#include <QFont>
#include <QIcon>

#include "ui/MainWindow.h"
#include "util/Log.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("CTFToolbox"));
    QApplication::setOrganizationName(QStringLiteral("CTFToolbox"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    QFont f = app.font();
    f.setFamily(QStringLiteral("Microsoft YaHei UI"));
    f.setPointSize(10);
    app.setFont(f);

    Log::init();

    QFile qss(QStringLiteral(":/qss/dark.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow w;
    w.show();

    return app.exec();
}
