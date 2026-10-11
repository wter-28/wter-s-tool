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
    for (const QJsonValue &v : tools)
        m_tools << ToolParse::fromJson(v.toObject());

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
                    m_tools[idx] = merged;
                }
            } else {
                ToolInfo t = ToolParse::fromJson(to);
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
