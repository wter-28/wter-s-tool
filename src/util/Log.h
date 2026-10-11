#pragma once

#include <QString>

// 轻量日志：输出到调试流并追加写入日志文件
namespace Log {
void init();
void info(const QString &msg);
void warn(const QString &msg);
void error(const QString &msg);
QString logFilePath();
} // namespace Log
