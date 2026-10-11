#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// 工具类型：决定启动命令如何拼装
enum class ToolType {
    Exe,      // 直接运行可执行文件
    Bat,      // 批处理（cmd /c）
    Jar,      // java -jar
    Python,   // python script.py
    Html,     // 用系统浏览器打开
    Folder,   // 仅打开目录
    Unknown
};

// 启动方式
enum class LaunchKind {
    Gui,      // 分离启动（GUI 程序）
    Cli,      // 进内置命令行运行器（流式输出）
    OpenUrl,  // 用默认程序/浏览器打开
    Folder    // 打开所在目录
};

// 运行时状态
enum class ToolState {
    Ok,               // 路径存在，可直接用
    Missing,          // 路径不存在
    NeedCompile,      // 只有源码，需要自行编译
    ExternalOk,       // 外部路径工具，存在
    ExternalMissing   // 外部路径工具，不存在
};

struct ToolExample {
    QString cmd;
    QString desc;
};

// 命令大全中的一条命令（可带分组，用于分区展示）
struct ToolCommand {
    QString group;   // 分组名（可空）
    QString cmd;     // 命令文本
    QString desc;    // 中文说明
};

// 工具的前置输入文件：运行前需准备并放到指定位置的文件
struct ToolInput {
    QString label;                 // 中文标签，如「USB 鼠标流量文本」
    QString accepts;               // 文件过滤器，如「文本文件 (*.txt)」
    QString dest;                  // 目标文件名（相对工具工作目录），如 out.txt
    QString note;                  // 说明 / 提示
    bool optional = false;         // 是否可选
};

struct ToolInfo {
    // ---- 静态元数据（来自 tools.json） ----
    QString id;
    QString name;
    QString categoryId;
    ToolType type = ToolType::Exe;
    LaunchKind launchKind = LaunchKind::Gui;
    QString execTarget;              // 相对工具根目录的路径；external 时为绝对路径
    QString workdir;                 // 相对工具根目录的工作目录（可空）
    QStringList defaultArgs;         // 默认参数
    QString argsPlaceholder;         // 运行器参数输入框的提示文本
    bool external = false;           // 路径在工具根目录之外
    bool requiresBuild = false;      // 仅源码，需要编译
    QString buildNote;               // 编译指引
    bool needAdmin = false;          // 需要管理员权限启动
    bool runnable = false;           // 是否可用内置命令行运行器
    QStringList deps;                // 依赖：java / python / npcap
    QStringList tags;
    QString website;
    QString description;             // 中文简介
    QString usage;                   // 中文使用说明
    QList<ToolExample> examples;
    QList<ToolCommand> commands;     // 命令大全（按 group 分组）
    QList<ToolInput> inputs;         // 运行前需要准备的前置文件
    QStringList tips;                // 注意事项 / CTF 技巧
    QString icon;                    // 图标名（可选）
    bool hidden = false;

    // ---- 运行时解析结果 ----
    QString resolvedPath;
    ToolState state = ToolState::Missing;

    bool ok() const { return state == ToolState::Ok || state == ToolState::ExternalOk; }
    bool needsConsole() const { return runnable && launchKind == LaunchKind::Cli; }

    // 是否存在需要用户准备的前置文件
    bool hasInputs() const { return !inputs.isEmpty(); }
    bool hasRequiredInputs() const
    {
        for (const ToolInput &in : inputs) {
            if (!in.optional)
                return true;
        }
        return false;
    }
};

struct CategoryInfo {
    QString id;
    QString name;
    QString icon;
    QString desc;
};

namespace ToolParse {
ToolType typeFromString(const QString &s);
LaunchKind launchKindFromString(const QString &s);
CategoryInfo categoryFromJson(const QJsonObject &o);
ToolInfo fromJson(const QJsonObject &o);
} // namespace ToolParse
