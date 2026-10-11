# wter's tool

> 基于 C++/Qt6 的 CTF / 渗透测试工具箱（以 TscanPlus 为蓝本），把分散的安全工具聚合到一个界面里，统一浏览、讲解与启动。

## 特性

- **数据驱动**：新增工具只需编辑 `resources/data/tools.json`，无需改代码
- **分类浏览 + 搜索**：每个工具含中文说明、使用示例、命令大全与提示
- **一键启动**：支持 exe / bat / jar / python / html / 目录，可按需以管理员权限启动（runas）
- **命令行运行**：内置控制台流式输出，支持交互输入、随时停止
- **前置文件拖拽**：需要外部输入的工具（如 USB 鼠标流量分析）会弹窗拖拽文件并自动落位
- **便携识别**：exe 同级存在 `tools` 目录时自动作为工具根目录，免配置
- **绿色免安装**：发布产物自带 Qt / MinGW 运行库

## 下载使用

1. 到 [Releases](../../releases) 下载 `wters-tool-1.0.0-win64.zip` 并解压（**不含任何工具**）
2. 运行 `wter's tool.exe`
3. 指定你的工具根目录，二选一：
   - 在弹窗 /「设置」里选择工具根目录（例如 `D:/MyDesktop/tools/ctf/tools`）
   - 或把你的 `tools` 目录直接放到 `wter's tool.exe` 同级，程序会自动识别（便携模式）

> 本仓库与发布包均**不包含**第三方安全工具，请自行准备并遵守各自许可。

## 从源码构建

依赖：Qt 6.10.2 (mingw_64)、CMake >= 3.21、Ninja、MinGW g++。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/QT/6.10.2/mingw_64"
cmake --build build
```

一键生成绿色便携包（含 Qt 运行库）：

```powershell
.\build_release.ps1
```

若要连同工具目录一起打包成自包含包：

```powershell
.\build_release.ps1 -BundleTools -ToolsSource "D:\MyDesktop\tools\ctf\tools"
```

## 目录结构

```
src/core    数据模型、路径解析、工具注册/启动、环境检测
src/ui      主窗口、分类侧栏、工具卡片/网格、详情页、拖拽弹窗
resources   tools.json 数据源、QSS 主题、分类图标
```

## 免责声明

仅供已获得合法授权的安全测试与学习使用，使用者需自行承担合规责任。