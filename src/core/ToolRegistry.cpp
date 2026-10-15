#include "ToolRegistry.h"
#include "PathResolver.h"
#include "util/LnkResolver.h"
#include "util/Log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

ToolRegistry::ToolRegistry(QObject *parent)
    : QObject(parent)
{
}

QString ToolRegistry::userOverridePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/user_tools.json");
}

static bool readJsonFile(const QString &path, QJsonObject *out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return false;
    *out = doc.object();
    return true;
}

void ToolRegistry::load()
{
    m_tools.clear();
    m_categories.clear();
    m_builtinIds.clear();

    QJsonObject root;
    if (!readJsonFile(QStringLiteral(":/data/tools.json"), &root)) {
        Log::error(QStringLiteral("无法读取内置工具清单 :/data/tools.json"));
        emit reloaded();
        return;
    }

    const QJsonArray cats = root.value(QStringLiteral("categories")).toArray();
    for (const QJsonValue &v : cats)
        m_categories << ToolParse::categoryFromJson(v.toObject());

    const QJsonArray tools = root.value(QStringLiteral("tools")).toArray();
    for (const QJsonValue &v : tools) {
        ToolInfo t = ToolParse::fromJson(v.toObject());
        m_builtinIds << t.id;
        m_tools << t;
    }

    // ---- 合并用户覆盖 ----
    QJsonObject uo;
    if (readJsonFile(userOverridePath(), &uo)) {
        const QJsonArray ucats = uo.value(QStringLiteral("categories")).toArray();
        for (const QJsonValue &v : ucats) {
            CategoryInfo c = ToolParse::categoryFromJson(v.toObject());
            if (c.id.isEmpty())
                continue;
            bool found = false;
            for (const CategoryInfo &existing : m_categories) {
                if (existing.id == c.id) { found = true; break; }
            }
            if (!found)
                m_categories << c;
        }

        const QJsonArray utools = uo.value(QStringLiteral("tools")).toArray();
        for (const QJsonValue &v : utools) {
            const QJsonObject to = v.toObject();
            const QString id = to.value(QStringLiteral("id")).toString();
            if (id.isEmpty())
                continue;

            int idx = -1;
            for (int i = 0; i < m_tools.size(); ++i) {
                if (m_tools.at(i).id == id) { idx = i; break; }
            }
            if (idx >= 0) {
                // 覆盖：隐藏 或 替换
                if (to.value(QStringLiteral("hidden")).toBool(false)) {
                    m_tools[idx].hidden = true;
                } else {
                    ToolInfo merged = ToolParse::fromJson(to);
                    merged.custom = !m_builtinIds.contains(merged.id);
                    merged.overridden = m_builtinIds.contains(merged.id);
                    m_tools[idx] = merged;
                }
            } else {
                ToolInfo t = ToolParse::fromJson(to);
                t.custom = true;
                if (!t.hidden)
                    m_tools << t;
            }
        }
    }

    Log::info(QStringLiteral("工具清单已加载：%1 个工具 / %2 个分类")
                  .arg(m_tools.size())
                  .arg(m_categories.size()));
    emit reloaded();
}

void ToolRegistry::setRootDir(const QString &dir)
{
    m_root = PathResolver::normalize(dir);
}

static ToolState resolveOne(ToolInfo &t, const QString &root)
{
    if (t.requiresBuild)
        return ToolState::NeedCompile;

    QString path;
    if (t.external || t.execTarget.isEmpty()) {
        path = t.external ? PathResolver::normalize(t.execTarget) : QString();
    } else {
        path = PathResolver::join(root, t.execTarget);
    }

    // 若目标本身是快捷方式，尝试解析其真实目标（兜底）
    if (!path.isEmpty() && path.endsWith(QStringLiteral(".lnk"), Qt::CaseInsensitive)) {
        const QString real = LnkResolver::resolve(path);
        if (!real.isEmpty())
            path = real;
    }

    t.resolvedPath = path;

    const bool ex = !path.isEmpty() && PathResolver::exists(path);
    if (t.external)
        return ex ? ToolState::ExternalOk : ToolState::ExternalMissing;
    return ex ? ToolState::Ok : ToolState::Missing;
}

void ToolRegistry::resolveAll()
{
    for (ToolInfo &t : m_tools)
        t.state = resolveOne(t, m_root);
    emit reloaded();
}

QList<ToolInfo> ToolRegistry::visibleTools() const
{
    QList<ToolInfo> out;
    for (const ToolInfo &t : m_tools) {
        if (!t.hidden)
            out << t;
    }
    return out;
}

const ToolInfo *ToolRegistry::findById(const QString &id) const
{
    for (const ToolInfo &t : m_tools) {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

int ToolRegistry::countInCategory(const QString &categoryId) const
{
    int n = 0;
    for (const ToolInfo &t : m_tools) {
        if (!t.hidden && t.categoryId == categoryId)
            ++n;
    }
    return n;
}

int ToolRegistry::totalVisibleCount() const
{
    int n = 0;
    for (const ToolInfo &t : m_tools) {
        if (!t.hidden)
            ++n;
    }
    return n;
}

bool ToolRegistry::isBuiltinId(const QString &id) const
{
    return m_builtinIds.contains(id);
}

// 把用户覆盖文档写回 user_tools.json（UTF-8）
static bool writeUserDoc(const QJsonObject &doc, QString *err)
{
    const QString path = ToolRegistry::userOverridePath();
    const QString dir = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(dir)) {
        if (err)
            *err = QStringLiteral("无法创建配置目录：%1").arg(dir);
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err)
            *err = QStringLiteral("无法写入 %1：%2").arg(path, f.errorString());
        return false;
    }
    f.write(QJsonDocument(doc).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool ToolRegistry::upsertUserTool(const ToolInfo &tool, QString *err)
{
    if (tool.id.isEmpty()) {
        if (err)
            *err = QStringLiteral("工具 id 不能为空");
        return false;
    }

    QJsonObject doc;
    readJsonFile(userOverridePath(), &doc);

    QJsonArray tools = doc.value(QStringLiteral("tools")).toArray();
    const QJsonObject entry = ToolParse::toJson(tool);

    bool replaced = false;
    for (int i = 0; i < tools.size(); ++i) {
        if (tools.at(i).toObject().value(QStringLiteral("id")).toString() == tool.id) {
            tools.replace(i, entry);
            replaced = true;
            break;
        }
    }
    if (!replaced)
        tools.append(entry);

    doc.insert(QStringLiteral("tools"), tools);
    if (!writeUserDoc(doc, err))
        return false;

    load();
    resolveAll();
    return true;
}

bool ToolRegistry::removeUserTool(const QString &id, QString *err)
{
    QJsonObject doc;
    if (!readJsonFile(userOverridePath(), &doc))
        return true;   // 没有用户文件，视为已恢复

    QJsonArray tools = doc.value(QStringLiteral("tools")).toArray();
    QJsonArray kept;
    for (const QJsonValue &v : tools) {
        if (v.toObject().value(QStringLiteral("id")).toString() == id)
            continue;
        kept.append(v);
    }
    doc.insert(QStringLiteral("tools"), kept);
    if (!writeUserDoc(doc, err))
        return false;

    load();
    resolveAll();
    return true;
}

bool ToolRegistry::setUserHidden(const QString &id, bool hidden, QString *err)
{
    QJsonObject doc;
    readJsonFile(userOverridePath(), &doc);

    QJsonArray tools = doc.value(QStringLiteral("tools")).toArray();
    int idx = -1;
    for (int i = 0; i < tools.size(); ++i) {
        if (tools.at(i).toObject().value(QStringLiteral("id")).toString() == id) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        QJsonObject entry = tools.at(idx).toObject();
        if (hidden) {
            entry.insert(QStringLiteral("hidden"), true);
            tools.replace(idx, entry);
        } else {
            entry.remove(QStringLiteral("hidden"));
            // 若条目只剩 id，说明它只是「隐藏」标记，取消隐藏后直接移除
            if (entry.size() <= 1)
                tools.removeAt(idx);
            else
                tools.replace(idx, entry);
        }
    } else if (hidden) {
        QJsonObject entry;
        entry.insert(QStringLiteral("id"), id);
        entry.insert(QStringLiteral("hidden"), true);
        tools.append(entry);
    }

    doc.insert(QStringLiteral("tools"), tools);
    if (!writeUserDoc(doc, err))
        return false;

    load();
    resolveAll();
    return true;
}
