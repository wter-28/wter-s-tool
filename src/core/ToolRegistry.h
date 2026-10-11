#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include "ToolInfo.h"

// 工具注册表：加载内置 tools.json + 用户覆盖，并按工具根目录解析每个工具的可用状态
class ToolRegistry : public QObject
{
    Q_OBJECT
public:
    explicit ToolRegistry(QObject *parent = nullptr);

    // 重新加载内置清单与用户覆盖（不解析路径）
    void load();

    // 设置工具根目录（工具解析的基准路径）
    void setRootDir(const QString &dir);
    QString rootDir() const { return m_root; }

    // 依据当前根目录解析所有工具的存在状态
    void resolveAll();

    QList<CategoryInfo> categories() const { return m_categories; }
    QList<ToolInfo> visibleTools() const;
    const ToolInfo *findById(const QString &id) const;

    int countInCategory(const QString &categoryId) const;
    int totalVisibleCount() const;

    // 用户覆盖文件路径：%APPDATA%/CTFToolbox/user_tools.json
    static QString userOverridePath();

signals:
    void reloaded();

private:
    QList<ToolInfo> m_tools;
    QList<CategoryInfo> m_categories;
    QString m_root;
};
