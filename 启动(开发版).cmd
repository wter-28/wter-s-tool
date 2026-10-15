@echo off
rem Dev launcher: add Qt runtime to PATH, then start the build-dev executable
set "PATH=D:\QT\6.10.2\mingw_64\bin;%PATH%"
start "" "%~dp0build-dev\wter's tool.exe"
