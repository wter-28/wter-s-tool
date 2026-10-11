#pragma once

#include <QLineEdit>

// 顶部搜索框
class SearchBar : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);

signals:
    void queryChanged(const QString &text);
};
