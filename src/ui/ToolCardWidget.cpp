#include "ToolCardWidget.h"

#include "core/PathResolver.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QStyle>
#include <QVBoxLayout>

namespace {
QString categoryIconPath(const QString &categoryId)
{
    return QStringLiteral(":/icons/cat_%1.svg").arg(categoryId);
}

const char *stateText(ToolState s)
{
    switch (s) {
    case ToolState::Ok:
    case ToolState::ExternalOk:
        return "可用";
    case ToolState::NeedCompile:
        return "需编译";
    case ToolState::ExternalMissing:
        return "路径缺失";
    case ToolState::Missing:
    default:
        return "缺失";
    }
}

QString stateName(ToolState s)
{
    switch (s) {
    case ToolState::Ok:
    case ToolState::ExternalOk:
        return QStringLiteral("ok");
    case ToolState::NeedCompile:
        return QStringLiteral("compile");
    case ToolState::ExternalMissing:
    case ToolState::Missing:
    default:
        return QStringLiteral("missing");
    }
}
} // namespace

ToolCardWidget::ToolCardWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ToolCard"));
    setFixedSize(268, 104);
    setCursor(Qt::PointingHandCursor);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_icon = new QLabel(this);
    m_icon->setObjectName(QStringLiteral("CardIcon"));
    m_icon->setFixedSize(40, 40);
    m_icon->setAlignment(Qt::AlignCenter);
    root->addWidget(m_icon, 0, Qt::AlignTop);

    auto *col = new QVBoxLayout();
    col->setSpacing(3);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(6);
    m_name = new QLabel(this);
    m_name->setObjectName(QStringLiteral("CardName"));
    m_badge = new QLabel(this);
    m_badge->setObjectName(QStringLiteral("CardBadge"));
    m_badge->setAlignment(Qt::AlignCenter);
    topRow->addWidget(m_name, 1);
    topRow->addWidget(m_badge, 0);
    col->addLayout(topRow);

    m_category = new QLabel(this);
    m_category->setObjectName(QStringLiteral("CardCategory"));
    col->addWidget(m_category);

    m_desc = new QLabel(this);
    m_desc->setObjectName(QStringLiteral("CardDesc"));
    col->addWidget(m_desc);
    col->addStretch();

    root->addLayout(col, 1);
}

void ToolCardWidget::setTool(const ToolInfo &tool)
{
    m_id = tool.id;

    const QString iconPath = tool.icon.isEmpty()
                                 ? categoryIconPath(tool.categoryId)
                                 : QStringLiteral(":/icons/%1").arg(tool.icon);
    QIcon icon(iconPath);
    if (icon.isNull())
        icon = QIcon(QStringLiteral(":/icons/app.svg"));
    m_icon->setPixmap(icon.pixmap(40, 40));

    m_name->setText(tool.name);
    m_name->setToolTip(tool.name);

    m_badge->setText(QString::fromUtf8(stateText(tool.state)));
    m_badge->setProperty("state", stateName(tool.state));
    m_badge->style()->unpolish(m_badge);
    m_badge->style()->polish(m_badge);

    m_category->setText(tool.tags.isEmpty() ? tool.categoryId
                                            : tool.tags.mid(0, 3).join(QStringLiteral(" · ")));

    const QString plain = tool.description.simplified();
    const QFontMetrics fm(m_desc->font());
    m_desc->setText(fm.elidedText(plain, Qt::ElideRight, 190));
    m_desc->setToolTip(plain);

    setToolTip(QStringLiteral("%1\n%2").arg(tool.name, plain));
}

void ToolCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_id.isEmpty())
        emit activated(m_id);
    QFrame::mousePressEvent(event);
}
