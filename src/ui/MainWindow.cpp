#include "MainWindow.h"
#include "DragDropInputDialog.h"
#include "FirstRunWizard.h"
#include "SearchBar.h"
#include "SidebarWidget.h"
#include "ToolDetailPage.h"
#include "ToolGridWidget.h"

#include "util/Log.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    wireSignals();

    AppSettings &cfg = AppSettings::instance();
    m_registry.setRootDir(cfg.toolsRoot());
    m_registry.load();
    m_registry.resolveAll();

    if (!cfg.firstRunDone() && AppSettings::portableToolsRoot().isEmpty())
        openSettings();

    m_env.detectAsync();
    showGridPage();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("wter's tool"));
    setMinimumSize(880, 560);
    const QRect avail = screen() ? screen()->availableGeometry() : QRect(0, 0, 1280, 800);
    resize(qMin(1200, avail.width() - 60), qMin(780, avail.height() - 60));

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部栏
    auto *header = new QWidget(central);
    header->setObjectName(QStringLiteral("HeaderBar"));
    auto *h = new QHBoxLayout(header);
    h->setContentsMargins(16, 10, 16, 10);
    h->setSpacing(12);

    auto *appTitle = new QLabel(QStringLiteral("wter's tool"), header);
    appTitle->setObjectName(QStringLiteral("AppTitle"));
    h->addWidget(appTitle);

    m_search = new SearchBar(header);
    h->addWidget(m_search, 1);

    m_envLabel = new QLabel(header);
    m_envLabel->setObjectName(QStringLiteral("Muted"));
    h->addWidget(m_envLabel);

    auto *settingsBtn = new QPushButton(QStringLiteral("设置"), header);
    settingsBtn->setObjectName(QStringLiteral("GhostButton"));
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::openSettings);
    h->addWidget(settingsBtn);
    root->addWidget(header);

    // 主体
    m_stack = new QStackedWidget(central);

    auto *gridPage = new QWidget(m_stack);
    auto *gh = new QHBoxLayout(gridPage);
    gh->setContentsMargins(0, 0, 0, 0);
    gh->setSpacing(0);
    m_sidebar = new SidebarWidget(gridPage);
    m_grid = new ToolGridWidget(gridPage);
    gh->addWidget(m_sidebar);
    gh->addWidget(m_grid, 1);

    m_detail = new ToolDetailPage(m_stack);

    m_stack->addWidget(gridPage);
    m_stack->addWidget(m_detail);
    root->addWidget(m_stack, 1);

    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::wireSignals()
{
    connect(m_sidebar, &SidebarWidget::categoryChanged, this, &MainWindow::onCategoryChanged);
    connect(m_search, &SearchBar::queryChanged, this, &MainWindow::onSearchChanged);
    connect(m_grid, &ToolGridWidget::toolActivated, this, &MainWindow::onToolActivated);
    connect(m_grid, &ToolGridWidget::countChanged, this, [this](int visible, int total) {
        statusBar()->showMessage(QStringLiteral("显示 %1 / %2 个工具").arg(visible).arg(total));
    });

    connect(m_detail, &ToolDetailPage::backRequested, this, &MainWindow::showGridPage);
    connect(m_detail, &ToolDetailPage::launchRequested, this, &MainWindow::onLaunch);
    connect(m_detail, &ToolDetailPage::runRequested, this, &MainWindow::onRun);
    connect(m_detail, &ToolDetailPage::prepareInputsRequested, this, &MainWindow::onPrepareInputs);
    connect(m_detail, &ToolDetailPage::stopRequested, this, &MainWindow::onStop);
    connect(m_detail, &ToolDetailPage::stdinSubmitted, &m_launcher, &ToolLauncher::writeStdin);
    connect(m_detail, &ToolDetailPage::openFolderRequested, this, &MainWindow::onOpenFolder);
    connect(m_detail, &ToolDetailPage::openUrlRequested, this, &MainWindow::onOpenUrl);

    connect(&m_launcher, &ToolLauncher::outputReady, m_detail, &ToolDetailPage::appendConsole);
    connect(&m_launcher, &ToolLauncher::started, this, [this](const QString &) {
        m_detail->setRunning(true);
    });
    connect(&m_launcher, &ToolLauncher::finished, this, [this](int code, bool crashed) {
        m_detail->setRunning(false);
        m_detail->appendConsole(QStringLiteral("\n[进程结束] 退出码 %1%2\n")
                                    .arg(code)
                                    .arg(crashed ? QStringLiteral("（异常退出）") : QString()));
    });

    connect(&m_env, &EnvDetector::detected, this, &MainWindow::onEnvDetected);
    connect(&m_registry, &ToolRegistry::reloaded, this, &MainWindow::refreshUi);
    connect(&AppSettings::instance(), &AppSettings::toolsRootChanged, this,
            [this](const QString &) { reloadRegistry(); });
}

void MainWindow::reloadRegistry()
{
    const AppSettings &cfg = AppSettings::instance();
    m_registry.setRootDir(cfg.toolsRoot());
    m_registry.load();
    m_registry.resolveAll();
}

void MainWindow::refreshUi()
{
    const QList<CategoryInfo> cats = m_registry.categories();
    QHash<QString, int> counts;
    for (const CategoryInfo &c : cats)
        counts.insert(c.id, m_registry.countInCategory(c.id));

    m_sidebar->setCategories(cats, counts, m_registry.totalVisibleCount());
    m_grid->setTools(m_registry.visibleTools());

    if (!m_registry.rootDir().isEmpty())
        statusBar()->showMessage(QStringLiteral("工具根目录：%1").arg(m_registry.rootDir()));
}

void MainWindow::onCategoryChanged(const QString &categoryId)
{
    m_grid->setCategoryFilter(categoryId);
}

void MainWindow::onSearchChanged(const QString &text)
{
    m_grid->setTextFilter(text);
    if (m_stack->currentWidget() != m_grid->parentWidget())
        showGridPage();
}

void MainWindow::onToolActivated(const QString &id)
{
    const ToolInfo *tool = m_registry.findById(id);
    if (!tool)
        return;
    m_detail->loadTool(*tool, m_envStatus);
    m_stack->setCurrentWidget(m_detail);
    AppSettings::instance().setLastToolId(id);
}

void MainWindow::showGridPage()
{
    if (m_stack->count() > 0)
        m_stack->setCurrentIndex(0);
}

void MainWindow::onPrepareInputs(const QString &id)
{
    const ToolInfo *tool = m_registry.findById(id);
    if (!tool || !tool->hasInputs())
        return;
    DragDropInputDialog::prepare(*tool, ToolLauncher::workingDirectory(*tool), this);
}

void MainWindow::onLaunch(const QString &id)
{
    const ToolInfo *tool = m_registry.findById(id);
    if (!tool)
        return;
    if (!DragDropInputDialog::prepare(*tool, ToolLauncher::workingDirectory(*tool), this))
        return;
    QString err;
    if (!m_launcher.launch(*tool, &err))
        QMessageBox::warning(this, QStringLiteral("启动失败"), err);
}

void MainWindow::onRun(const QString &id, const QStringList &args)
{
    const ToolInfo *tool = m_registry.findById(id);
    if (!tool)
        return;
    if (!DragDropInputDialog::prepare(*tool, ToolLauncher::workingDirectory(*tool), this))
        return;
    m_detail->clearConsole();
    QString err;
    if (!m_launcher.startStreaming(*tool, args, &err)) {
        m_detail->appendConsole(QStringLiteral("[错误] %1\n").arg(err));
        QMessageBox::warning(this, QStringLiteral("运行失败"), err);
    }
}

void MainWindow::onStop()
{
    m_launcher.stop();
}

void MainWindow::onOpenFolder(const QString &path)
{
    ToolLauncher::openFolder(path);
}

void MainWindow::onOpenUrl(const QString &url)
{
    if (!url.isEmpty())
        ToolLauncher::openUrl(url);
}

void MainWindow::openSettings()
{
    AppSettings &cfg = AppSettings::instance();
    FirstRunWizard wiz(!cfg.firstRunDone(), this);
    if (wiz.exec() != QDialog::Accepted)
        return;

    cfg.setJavaPath(wiz.javaPath());
    cfg.setPythonPath(wiz.pythonPath());
    cfg.setFirstRunDone(true);
    cfg.setToolsRoot(wiz.toolsRoot());   // 触发 toolsRootChanged → reloadRegistry

    reloadRegistry();
    m_env.detectAsync();
    Log::info(QStringLiteral("设置已更新，工具根目录：%1").arg(cfg.toolsRoot()));
}

void MainWindow::onEnvDetected(const EnvStatus &status)
{
    m_envStatus = status;
    m_detail->setEnv(status);
    updateEnvLabel();
}

void MainWindow::updateEnvLabel()
{
    auto mark = [](bool ok) { return ok ? QStringLiteral("●") : QStringLiteral("○"); };
    m_envLabel->setText(QStringLiteral("Java %1  Python %2  Npcap %3")
                            .arg(mark(m_envStatus.javaOk),
                                 mark(m_envStatus.pythonOk),
                                 mark(m_envStatus.npcapOk)));
    m_envLabel->setToolTip(QStringLiteral("Java: %1\nPython: %2\nNpcap: %3")
                               .arg(m_envStatus.javaPath.isEmpty() ? QStringLiteral("未检测到") : m_envStatus.javaPath,
                                    m_envStatus.pythonPath.isEmpty() ? QStringLiteral("未检测到") : m_envStatus.pythonPath,
                                    m_envStatus.npcapOk ? QStringLiteral("已安装") : QStringLiteral("未安装")));
}
