cmake_minimum_required(VERSION 3.21)

project(wters_tool VERSION 1.0.0 LANGUAGES CXX
        DESCRIPTION "CTF / 渗透测试工具箱 —— 以 TscanPlus 为蓝本的工具聚合客户端")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 默认 Qt 前缀（可在命令行用 -DCMAKE_PREFIX_PATH=... 覆盖）
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/QT/6.10.2/mingw_64" CACHE STRING "Qt6 安装前缀")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Svg)

qt_standard_project_setup()

# 调试用：-DCTFBOX_CONSOLE=ON 可保留控制台窗口
option(CTFBOX_CONSOLE "构建带控制台窗口的版本（便于调试）" OFF)

set(PROJECT_SOURCES
    src/main.cpp

    src/core/ToolInfo.h
    src/core/ToolInfo.cpp
    src/core/PathResolver.h
    src/core/PathResolver.cpp
    src/core/ToolRegistry.h
    src/core/ToolRegistry.cpp
    src/core/ToolLauncher.h
    src/core/ToolLauncher.cpp
    src/core/AppSettings.h
    src/core/AppSettings.cpp
    src/core/EnvDetector.h
    src/core/EnvDetector.cpp

    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/SidebarWidget.h
    src/ui/SidebarWidget.cpp
    src/ui/SearchBar.h
    src/ui/SearchBar.cpp
    src/ui/ToolCardWidget.h
    src/ui/ToolCardWidget.cpp
    src/ui/ToolGridWidget.h
    src/ui/ToolGridWidget.cpp
    src/ui/ToolDetailPage.h
    src/ui/ToolDetailPage.cpp
    src/ui/ConsoleWidget.h
    src/ui/ConsoleWidget.cpp
    src/ui/DragDropInputDialog.h
    src/ui/DragDropInputDialog.cpp
    src/ui/FirstRunWizard.h
    src/ui/FirstRunWizard.cpp

    src/util/LnkResolver.h
    src/util/LnkResolver.cpp
    src/util/Log.h
    src/util/Log.cpp

    resources/app.qrc
)

if(CTFBOX_CONSOLE)
    qt_add_executable(wters_tool ${PROJECT_SOURCES})
else()
    qt_add_executable(wters_tool WIN32 ${PROJECT_SOURCES})
endif()

# 产物文件名：显示为 wter's tool.exe（目标名保持合法标识符）
set_target_properties(wters_tool PROPERTIES OUTPUT_NAME "wter's tool")

target_include_directories(wters_tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(wters_tool PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(wters_tool PRIVATE ole32 shell32 user32 advapi32)
endif()

# 安装/打包（配合 windeployqt 使用，见 build_release.ps1）
install(TARGETS wters_tool RUNTIME DESTINATION .)
