#include "DragDropInputDialog.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

// ---------------- DropZone ----------------

DropZone::DropZone(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("DropZone"));
    setAcceptDrops(true);
    setMinimumHeight(64);
    setCursor(Qt::PointingHandCursor);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(12, 10, 12, 10);

    auto *label = new QLabel(this);
    label->setObjectName(QStringLiteral("DropZoneText"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    lay->addWidget(label);

    updateText();
}

void DropZone::setFilter(const QString &filter)
{
    m_filter = filter;
}

void DropZone::setSource(const QString &path)
{
    m_source = path;
    updateText();
    setProperty("filled", !path.isEmpty());
    style()->unpolish(this);
    style()->polish(this);
    if (m_onChanged)
        m_onChanged(path);
}

void DropZone::setFromUrl(const QUrl &url)
{
    if (url.isLocalFile())
        setSource(url.toLocalFile());
}

void DropZone::updateText()
{
    auto *label = findChild<QLabel *>(QStringLiteral("DropZoneText"));
    if (!label)
        return;
    if (m_source.isEmpty()) {
        label->setText(QStringLiteral("拖拽文件到此处，或点击选择"));
        setToolTip(QString());
    } else {
        const QFileInfo fi(m_source);
        const double kb = fi.size() / 1024.0;
        label->setText(QStringLiteral("已选择：%1  （%2 KB）")
                           .arg(fi.fileName())
                           .arg(QString::number(kb, 'f', 1)));
        setToolTip(m_source);
    }
}

void DropZone::choose()
{
    const QString filter = m_filter.isEmpty()
                               ? QStringLiteral("所有文件 (*)")
                               : m_filter + QStringLiteral(";;所有文件 (*)");
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"),
                                                      QString(), filter);
    if (!file.isEmpty())
        setSource(file);
}

void DropZone::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
        m_hover = true;
        setProperty("hover", true);
        style()->unpolish(this);
        style()->polish(this);
    }
}

void DropZone::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
    m_hover = false;
    setProperty("hover", false);
    style()->unpolish(this);
    style()->polish(this);
}

void DropZone::dropEvent(QDropEvent *e)
{
    m_hover = false;
    setProperty("hover", false);
    style()->unpolish(this);
    style()->polish(this);

    const QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty())
        return;
    setFromUrl(urls.first());
    e->acceptProposedAction();
}

void DropZone::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        choose();
    QFrame::mousePressEvent(e);
}

// ---------------- DragDropInputDialog ----------------

DragDropInputDialog::DragDropInputDialog(const ToolInfo &tool, const QString &workdir, QWidget *parent)
    : QDialog(parent)
    , m_workdir(workdir)
{
    setWindowTitle(QStringLiteral("准备前置文件 —— %1").arg(tool.name));
    setModal(true);
    resize(560, 0);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(12);

    auto *intro = new QLabel(this);
    intro->setObjectName(QStringLiteral("BodyText"));
    intro->setWordWrap(true);
    intro->setText(QStringLiteral("「%1」在运行前需要以下文件。可直接把文件拖到方框内，"
                                  "或点击方框选择；确认后会自动复制到工具目录。")
                       .arg(tool.name));
    root->addWidget(intro);

    for (const ToolInput &in : tool.inputs) {
        Row row;
        row.input = in;
        row.destPath = QDir(m_workdir).absoluteFilePath(in.dest);

        auto *title = new QLabel(this);
        QString labelText = in.label.isEmpty() ? in.dest : in.label;
        if (in.optional)
            labelText += QStringLiteral("（可选）");
        title->setText(labelText);
        title->setObjectName(QStringLiteral("InputLabel"));
        root->addWidget(title);

        if (!in.note.isEmpty()) {
            auto *note = new QLabel(in.note, this);
            note->setObjectName(QStringLiteral("Muted"));
            note->setWordWrap(true);
            root->addWidget(note);
        }

        auto *zone = new DropZone(this);
        zone->setFilter(in.accepts);
        row.zone = zone;
        root->addWidget(zone);

        auto *status = new QLabel(this);
        status->setObjectName(QStringLiteral("Muted"));
        status->setWordWrap(true);
        row.status = status;
        root->addWidget(status);

        if (QFileInfo::exists(row.destPath)) {
            status->setText(QStringLiteral("目标已存在，可直接继续：%1").arg(QDir::toNativeSeparators(row.destPath)));
        } else {
            status->setText(QStringLiteral("目标位置：%1").arg(QDir::toNativeSeparators(row.destPath)));
        }

        const QString dest = in.dest;
        const QString destPath = row.destPath;
        zone->setOnChanged([status, dest, destPath](const QString &src) {
            if (src.isEmpty()) {
                if (QFileInfo::exists(destPath))
                    status->setText(QStringLiteral("目标已存在，可直接继续：%1").arg(QDir::toNativeSeparators(destPath)));
                else
                    status->setText(QStringLiteral("目标位置：%1").arg(QDir::toNativeSeparators(destPath)));
            } else {
                status->setText(QStringLiteral("将复制为：%1").arg(dest));
            }
        });

        m_rows.append(row);
    }

    root->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *cancel = new QPushButton(QStringLiteral("取消"), this);
    cancel->setObjectName(QStringLiteral("GhostButton"));
    auto *ok = new QPushButton(QStringLiteral("确认并继续"), this);
    ok->setObjectName(QStringLiteral("PrimaryButton"));
    btnRow->addWidget(cancel);
    btnRow->addWidget(ok);
    root->addLayout(btnRow);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, &DragDropInputDialog::tryAccept);
}

void DragDropInputDialog::tryAccept()
{
    for (const Row &row : m_rows) {
        const QString src = row.zone->source();
        if (src.isEmpty() && !row.input.optional && !QFileInfo::exists(row.destPath)) {
            QMessageBox::warning(this, QStringLiteral("还差一个文件"),
                                 QStringLiteral("请先准备「%1」，或把文件放到：\n%2")
                                     .arg(row.input.label.isEmpty() ? row.input.dest : row.input.label,
                                          QDir::toNativeSeparators(row.destPath)));
            return;
        }
    }

    QString err;
    if (!applyInputs(&err)) {
        QMessageBox::warning(this, QStringLiteral("准备失败"), err);
        return;
    }
    QDialog::accept();
}

bool DragDropInputDialog::applyInputs(QString *err)
{
    QDir dir(m_workdir);
    if (!dir.exists() && !QDir().mkpath(m_workdir)) {
        if (err)
            *err = QStringLiteral("无法创建工作目录：%1").arg(m_workdir);
        return false;
    }

    for (const Row &row : m_rows) {
        const QString src = row.zone->source();
        if (src.isEmpty())
            continue;   // 可选未提供，或目标已存在

        const QFileInfo srcInfo(src);
        if (!srcInfo.exists() || !srcInfo.isFile()) {
            if (err)
                *err = QStringLiteral("源文件不存在：%1").arg(src);
            return false;
        }

        const QString destPath = row.destPath;
        if (QFileInfo(srcInfo.absoluteFilePath()).absoluteFilePath()
            == QFileInfo(destPath).absoluteFilePath()) {
            continue;   // 同一文件，无需复制
        }

        if (QFileInfo::exists(destPath) && !QFile::remove(destPath)) {
            if (err)
                *err = QStringLiteral("无法覆盖已存在的文件：%1").arg(destPath);
            return false;
        }
        if (!QFile::copy(src, destPath)) {
            if (err)
                *err = QStringLiteral("复制失败：%1 → %2").arg(src, destPath);
            return false;
        }
    }
    return true;
}

bool DragDropInputDialog::prepare(const ToolInfo &tool, const QString &workdir,
                                  QWidget *parent, QString *err)
{
    if (!tool.hasInputs())
        return true;

    if (workdir.isEmpty()) {
        if (err)
            *err = QStringLiteral("无法确定工具工作目录，无法准备前置文件");
        return false;
    }

    DragDropInputDialog dlg(tool, workdir, parent);
    return dlg.exec() == QDialog::Accepted;
}
