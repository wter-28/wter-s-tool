#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

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
    QList<ToolInfo> allTools() const { return m_tools; }
    const ToolInfo *findById(const QString &id) const;

    // 是否为内置工具（区分「恢复默认」与「删除」语义）
    bool isBuiltinId(const QString &id) const;

    // 用户工具写操作：写入 user_tools.json 并重新加载，返回是否成功
    bool upsertUserTool(const ToolInfo &tool, QString *err = nullptr);
    bool removeUserTool(const QString &id, QString *err = nullptr);
    bool setUserHidden(const QString &id, bool hidden, QString *err = nullptr);

    int countInCategory(const QString &categoryId) const;
    int totalVisibleCount() const;

    // 用户覆盖文件路径：%APPDATA%/CTFToolbox/user_tools.json
    static QString userOverridePath();

signals:
    void reloaded();

private:
    QList<ToolInfo> m_tools;
    QList<CategoryInfo> m_categories;
    QStringList m_builtinIds;
    QString m_root;
};
