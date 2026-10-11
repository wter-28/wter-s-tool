#include "ToolInfo.h"

#include <QJsonArray>
#include <QJsonValue>

namespace ToolParse {

ToolType typeFromString(const QString &s)
{
    const QString t = s.trimmed().toLower();
    if (t == QStringLiteral("exe")) return ToolType::Exe;
    if (t == QStringLiteral("bat")) return ToolType::Bat;
    if (t == QStringLiteral("jar")) return ToolType::Jar;
    if (t == QStringLiteral("python")) return ToolType::Python;
    if (t == QStringLiteral("html")) return ToolType::Html;
    if (t == QStringLiteral("folder")) return ToolType::Folder;
    return ToolType::Unknown;
}

LaunchKind launchKindFromString(const QString &s)
{
    const QString t = s.trimmed().toLower();
    if (t == QStringLiteral("cli")) return LaunchKind::Cli;
    if (t == QStringLiteral("openurl")) return LaunchKind::OpenUrl;
    if (t == QStringLiteral("folder")) return LaunchKind::Folder;
    return LaunchKind::Gui;
}

static QStringList stringListFrom(const QJsonValue &v)
{
    QStringList out;
    if (v.isArray()) {
        const QJsonArray arr = v.toArray();
        for (const QJsonValue &item : arr) {
            const QString s = item.toString().trimmed();
            if (!s.isEmpty())
                out << s;
        }
    } else if (v.isString()) {
        const QString s = v.toString().trimmed();
        if (!s.isEmpty())
            out << s;
    }
    return out;
}

CategoryInfo categoryFromJson(const QJsonObject &o)
{
    CategoryInfo c;
    c.id = o.value(QStringLiteral("id")).toString();
    c.name = o.value(QStringLiteral("name")).toString(c.id);
    c.icon = o.value(QStringLiteral("icon")).toString(c.id);
    c.desc = o.value(QStringLiteral("desc")).toString();
    return c;
}

ToolInfo fromJson(const QJsonObject &o)
{
    ToolInfo t;
    t.id = o.value(QStringLiteral("id")).toString();
    t.name = o.value(QStringLiteral("name")).toString(t.id);
    t.categoryId = o.value(QStringLiteral("category")).toString();
    t.type = typeFromString(o.value(QStringLiteral("type")).toString());
    t.launchKind = launchKindFromString(o.value(QStringLiteral("launchKind")).toString());
    t.execTarget = o.value(QStringLiteral("execTarget")).toString();
    t.workdir = o.value(QStringLiteral("workdir")).toString();
    t.defaultArgs = stringListFrom(o.value(QStringLiteral("defaultArgs")));
    t.argsPlaceholder = o.value(QStringLiteral("argsPlaceholder")).toString();
    t.external = o.value(QStringLiteral("external")).toBool(false);
    t.requiresBuild = o.value(QStringLiteral("requiresBuild")).toBool(false);
    t.buildNote = o.value(QStringLiteral("buildNote")).toString();
    t.needAdmin = o.value(QStringLiteral("needAdmin")).toBool(false);
    t.runnable = o.value(QStringLiteral("runnable")).toBool(t.type != ToolType::Html
                                                            && t.launchKind != LaunchKind::Gui
                                                            && t.launchKind != LaunchKind::OpenUrl
                                                            && t.launchKind != LaunchKind::Folder);
    t.deps = stringListFrom(o.value(QStringLiteral("deps")));
    t.tags = stringListFrom(o.value(QStringLiteral("tags")));
    t.website = o.value(QStringLiteral("website")).toString();
    t.description = o.value(QStringLiteral("description")).toString();
    t.usage = o.value(QStringLiteral("usage")).toString();
    t.tips = stringListFrom(o.value(QStringLiteral("tips")));
    t.icon = o.value(QStringLiteral("icon")).toString();
    t.hidden = o.value(QStringLiteral("hidden")).toBool(false);

    const QJsonValue ex = o.value(QStringLiteral("examples"));
    if (ex.isArray()) {
        const QJsonArray arr = ex.toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject eo = v.toObject();
            ToolExample e;
            e.cmd = eo.value(QStringLiteral("cmd")).toString();
            e.desc = eo.value(QStringLiteral("desc")).toString();
            if (!e.cmd.isEmpty())
                t.examples << e;
        }
    }

    // 命令大全（commands）：每项可带 group 分组
    const QJsonValue cm = o.value(QStringLiteral("commands"));
    if (cm.isArray()) {
        const QJsonArray arr = cm.toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject co = v.toObject();
            ToolCommand c;
            c.group = co.value(QStringLiteral("group")).toString();
            c.cmd = co.value(QStringLiteral("cmd")).toString();
            c.desc = co.value(QStringLiteral("desc")).toString();
            if (!c.cmd.isEmpty())
                t.commands << c;
        }
    }

    // 前置输入文件（inputs）
    const QJsonValue in = o.value(QStringLiteral("inputs"));
    if (in.isArray()) {
        const QJsonArray arr = in.toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject io = v.toObject();
            ToolInput ti;
            ti.label = io.value(QStringLiteral("label")).toString();
            ti.accepts = io.value(QStringLiteral("accepts")).toString();
            ti.dest = io.value(QStringLiteral("dest")).toString();
            ti.note = io.value(QStringLiteral("note")).toString();
            ti.optional = io.value(QStringLiteral("optional")).toBool(false);
            if (!ti.dest.isEmpty())
                t.inputs << ti;
        }
    }
    return t;
}

} // namespace ToolParse
