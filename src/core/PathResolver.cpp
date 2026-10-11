#include "PathResolver.h"

#include <QDir>
#include <QFileInfo>

namespace PathResolver {

bool isAbsolute(const QString &p)
{
    if (p.isEmpty())
        return false;
    if (p.startsWith(QLatin1Char('/')) || p.startsWith(QLatin1Char('\\')))
        return true;
    // 形如 C:\ 或 C:/
    if (p.size() >= 2 && p.at(1) == QLatin1Char(':'))
        return true;
    return false;
}

QString normalize(const QString &p)
{
    QString s = p;
    s.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return QDir::cleanPath(s);
}

QString join(const QString &root, const QString &rel)
{
    if (rel.isEmpty())
        return normalize(root);
    if (isAbsolute(rel))
        return normalize(rel);
    const QString base = normalize(root);
    if (base.isEmpty())
        return normalize(rel);
    return normalize(base + QLatin1Char('/') + rel);
}

bool exists(const QString &p)
{
    if (p.isEmpty())
        return false;
    return QFileInfo::exists(p);
}

bool isDir(const QString &p)
{
    if (p.isEmpty())
        return false;
    return QFileInfo(p).isDir();
}

QString parentDir(const QString &p)
{
    const QFileInfo fi(p);
    if (fi.isDir())
        return fi.absoluteFilePath();
    return fi.absolutePath();
}

} // namespace PathResolver
