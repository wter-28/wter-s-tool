#include "FirstRunWizard.h"
#include "core/AppSettings.h"
#include "core/PathResolver.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

FirstRunWizard::FirstRunWizard(bool firstRun, QWidget *parent)
    : QDialog(parent)
    , m_firstRun(firstRun)
{
    setWindowTitle(firstRun ? QStringLiteral("欢迎使用 CTF 工具箱")
                            : QStringLiteral("设置"));
    setModal(true);
    setMinimumWidth(560);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto *intro = new QLabel(this);
    intro->setObjectName(QStringLiteral("BodyText"));
    intro->setWordWrap(true);
    intro->setText(firstRun
                       ? QStringLiteral("本工具箱通过「工具根目录」定位并启动你已有的安全工具，"
                                        "自身不复制任何工具文件。<br>请选择存放这些工具的目录（例如："
                                        "<code>D:/MyDesktop/tools/ctf/tools</code>）。")
                       : QStringLiteral("修改工具根目录或运行时路径后，点击「保存」生效。"));
    root->addWidget(intro);

    auto *form = new QFormLayout();
    form->setSpacing(8);

    auto makeRow = [this, form](const QString &label, const QString &placeholder, QLineEdit **out) {
        auto *row = new QWidget(this);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        auto *edit = new QLineEdit(row);
        edit->setPlaceholderText(placeholder);
        auto *browse = new QPushButton(QStringLiteral("浏览…"), row);
        h->addWidget(edit, 1);
        h->addWidget(browse);
        form->addRow(label, row);
        *out = edit;
        connect(browse, &QPushButton::clicked, this, [this, edit, placeholder]() {
            browseInto(edit, placeholder);
        });
    };

    makeRow(QStringLiteral("工具根目录"), QStringLiteral("例如 D:/MyDesktop/tools/ctf/tools"), &m_root);
    makeRow(QStringLiteral("Java 路径"), QStringLiteral("留空则自动从 PATH 查找"), &m_java);
    makeRow(QStringLiteral("Python 路径"), QStringLiteral("留空则自动从 PATH 查找"), &m_python);
    root->addLayout(form);

    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("Muted"));
    m_hint->setWordWrap(true);
    root->addWidget(m_hint);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *cancel = new QPushButton(QStringLiteral("取消"), this);
    m_ok = new QPushButton(QStringLiteral("保存"), this);
    m_ok->setObjectName(QStringLiteral("PrimaryButton"));
    btnRow->addWidget(cancel);
    btnRow->addWidget(m_ok);
    root->addLayout(btnRow);

    // 预填当前值
    const AppSettings &cfg = AppSettings::instance();
    m_root->setText(cfg.toolsRoot());
    m_java->setText(cfg.javaPath());
    m_python->setText(cfg.pythonPath());

    connect(m_ok, &QPushButton::clicked, this, [this]() {
        const QString r = PathResolver::normalize(m_root->text().trimmed());
        if (r.isEmpty() || !QFileInfo::exists(r)) {
            m_hint->setText(QStringLiteral("⚠ 工具根目录不存在，请重新选择。"));
            return;
        }
        accept();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_root, &QLineEdit::textChanged, this, &FirstRunWizard::validate);
    validate();
}

void FirstRunWizard::browseInto(QLineEdit *edit, const QString &title)
{
    const QString start = edit->text().isEmpty() ? QDir::homePath() : edit->text();
    const QString dir = QFileDialog::getExistingDirectory(this, title, start);
    if (!dir.isEmpty())
        edit->setText(PathResolver::normalize(dir));
}

void FirstRunWizard::validate()
{
    const QString r = PathResolver::normalize(m_root->text().trimmed());
    if (r.isEmpty()) {
        m_hint->setText(QString());
        return;
    }
    if (!QFileInfo::exists(r)) {
        m_hint->setText(QStringLiteral("⚠ 目录不存在。"));
        return;
    }
    // 轻量校验：看是否包含若干已知工具目录
    static const QStringList markers = {QStringLiteral("subfinder"), QStringLiteral("ida"),
                                        QStringLiteral("BurpSuite V2023.9.2"),
                                        QStringLiteral("ksubdomain.exe")};
    int hit = 0;
    for (const QString &m : markers) {
        if (QFileInfo::exists(r + QLatin1Char('/') + m))
            ++hit;
    }
    if (hit == 0)
        m_hint->setText(QStringLiteral("提示：该目录下未发现常见工具，仍可继续，但多数工具会显示为「缺失」。"));
    else
        m_hint->setText(QStringLiteral("已识别到 %1 个常见工具标记。").arg(hit));
}

QString FirstRunWizard::toolsRoot() const
{
    return PathResolver::normalize(m_root->text().trimmed());
}

QString FirstRunWizard::javaPath() const
{
    return m_java->text().trimmed();
}

QString FirstRunWizard::pythonPath() const
{
    return m_python->text().trimmed();
}
