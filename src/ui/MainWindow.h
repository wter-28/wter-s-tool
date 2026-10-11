#pragma once

#include <QMainWindow>

#include "core/AppSettings.h"
#include "core/EnvDetector.h"
#include "core/ToolLauncher.h"
#include "core/ToolRegistry.h"

class QLabel;
class QStackedWidget;
class SearchBar;
class SidebarWidget;
class ToolGridWidget;
class ToolDetailPage;

// 主窗口：侧栏分类 + 工具卡片网格 + 详情页
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onCategoryChanged(const QString &categoryId);
    void onSearchChanged(const QString &text);
    void onToolActivated(const QString &id);
    void showGridPage();
    void onLaunch(const QString &id);
    void onRun(const QString &id, const QStringList &args);
    void onPrepareInputs(const QString &id);
    void onStop();
    void onOpenFolder(const QString &path);
    void onOpenUrl(const QString &url);
    void openSettings();
    void onEnvDetected(const EnvStatus &status);
    void refreshUi();

private:
    void buildUi();
    void wireSignals();
    void reloadRegistry();
    void updateEnvLabel();

    ToolRegistry m_registry;
    ToolLauncher m_launcher;
    EnvDetector m_env;
    EnvStatus m_envStatus;

    SidebarWidget *m_sidebar = nullptr;
    SearchBar *m_search = nullptr;
    ToolGridWidget *m_grid = nullptr;
    ToolDetailPage *m_detail = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLabel *m_envLabel = nullptr;
};
