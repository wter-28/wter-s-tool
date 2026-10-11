#pragma once

#include <QString>

// 路径处理工具：统一处理「中文 / 空格 / 正反斜杠 / 绝对与相对」等情形
namespace PathResolver {

// 判断是否为绝对路径（Windows 盘符或 UNC / 以 / 开头）
bool isAbsolute(const QString &p);

// 将 root 与相对路径拼接；若 rel 已是绝对路径则原样返回
QString join(const QString &root, const QString &rel);

// 统一为正斜杠、去掉冗余分隔符
QString normalize(const QString &p);

bool exists(const QString &p);
bool isDir(const QString &p);

// 返回文件所在目录（对目录本身返回其父目录）
QString parentDir(const QString &p);

} // namespace PathResolver
