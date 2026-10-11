#pragma once

#include <QString>

// 解析 Windows 快捷方式（.lnk）的真实目标路径；仅作为兜底，
// tools.json 中应尽量直接写真实目标路径。
namespace LnkResolver {
QString resolve(const QString &lnkPath);
}
