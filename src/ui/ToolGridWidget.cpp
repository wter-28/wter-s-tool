#include "ToolGridWidget.h"
#include "ToolCardWidget.h"

#include <QGridLayout>
#include <QHash>
#include <QLabel>
#include <QLayoutItem>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
constexpr int kCardWidth = 268;
constexpr int kSpacing = 12;
} // namespace

ToolGridWidget::ToolGridWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("GridScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_container = new QWidget(m_scroll);
    m_grid = new QGridLayout(m_container);
    m_grid->setContentsMargins(4, 4, 4, 4);
    m_grid->setSpacing(kSpacing);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_empty = new QLabel(QStringLiteral("没有匹配的工具"), m_container);
    m_empty->setObjectName(QStringLiteral("EmptyHint"));
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->hide();
    m_grid->addWidget(m_empty, 0, 0);

    m_scroll->setWidget(m_container);
    m_scroll->viewport()->installEventFilter(this);
    root->addWidget(m_scroll);
}

void ToolGridWidget::setTools(const QList<ToolInfo> &tools)
{
    m_tools = tools;
    rebuildCards();
    applyFilter();
}

void ToolGridWidget::rebuildCards()
{
    // 清理旧卡片
    for (ToolCardWidget *card : m_cards) {
        m_grid->removeWidget(card);
        card->deleteLater();
    }
    m_cards.clear();
    m_visible.clear();

    for (const ToolInfo &t : m_tools) {
        auto *card = new ToolCardWidget(m_container);
        card->setTool(t);
        connect(card, &ToolCardWidget::activated, this, &ToolGridWidget::toolActivated);
        card->hide();
        m_cards.insert(t.id, card);
    }
}

void ToolGridWidget::setCategoryFilter(const QString &categoryId)
{
    m_category = categoryId;
    applyFilter();
}

void ToolGridWidget::setTextFilter(const QString &text)
{
    m_text = text.trimmed();
    applyFilter();
}

void ToolGridWidget::applyFilter()
{
    m_visible.clear();
    for (const ToolInfo &t : m_tools) {
        const bool catOk = (m_category == QStringLiteral("all")) || (t.categoryId == m_category);
        bool textOk = true;
        if (!m_text.isEmpty()) {
            const QString hay = (t.name + QLatin1Char(' ') + t.id + QLatin1Char(' ')
                                 + t.description + QLatin1Char(' ') + t.tags.join(QLatin1Char(' ')))
                                    .toLower();
            textOk = hay.contains(m_text.toLower());
        }
        if (catOk && textOk)
            m_visible << m_cards.value(t.id);
    }

    relayout();
    emit countChanged(m_visible.size(), m_tools.size());
}

int ToolGridWidget::columnCount() const
{
    const int avail = m_scroll->viewport()->width() - 8;
    const int cols = (avail + kSpacing) / (kCardWidth + kSpacing);
    return qMax(1, cols);
}

void ToolGridWidget::relayout()
{
    // 从布局中摘除全部项（不删除控件）
    while (QLayoutItem *item = m_grid->takeAt(0))
        delete item;

    const int cols = columnCount();
    int row = 0;
    int col = 0;
    for (ToolCardWidget *card : m_visible) {
        card->show();
        m_grid->addWidget(card, row, col);
        if (++col >= cols) {
            col = 0;
            ++row;
        }
    }

    // 隐藏未入选的卡片
    for (ToolCardWidget *card : m_cards) {
        if (!m_visible.contains(card))
            card->hide();
    }

    if (m_visible.isEmpty()) {
        m_empty->show();
        m_grid->addWidget(m_empty, 0, 0, 1, qMax(1, cols));
    } else {
        m_empty->hide();
    }

    m_grid->setRowStretch(row + 1, 1);
}

void ToolGridWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool ToolGridWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_scroll->viewport() && event->type() == QEvent::Resize)
        relayout();
    return QWidget::eventFilter(watched, event);
}
