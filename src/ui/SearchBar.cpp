#include "SearchBar.h"

#include <QAction>

SearchBar::SearchBar(QWidget *parent)
    : QLineEdit(parent)
{
    setObjectName(QStringLiteral("SearchBar"));
    setPlaceholderText(QStringLiteral("搜索工具名称、标签或用途…"));
    setClearButtonEnabled(true);
    setMinimumWidth(260);

    connect(this, &QLineEdit::textChanged, this, &SearchBar::queryChanged);
}
