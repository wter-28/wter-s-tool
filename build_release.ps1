<#
  wter's tool 发布打包脚本
  ------------------------------------------------------------------
  生成绿色免安装版：dist\wter's tool\
    - wter's tool.exe（Release）
    - Qt 运行库与 MinGW 运行库（由 windeployqt 自动收集）
    - LICENSE / README（如存在）

  用法（在项目根目录执行）：
    powershell -ExecutionPolicy Bypass -File build_release.ps1

  可用参数覆盖默认路径：
    -QtDir      Qt 前缀目录       （默认 D:\QT\6.10.2\mingw_64）
    -MingwBin   MinGW 的 bin 目录 （默认 D:\QT\Tools\mingw1310_64\bin）
    -Ninja      ninja.exe 路径    （默认 D:\QT\Tools\Ninja\ninja.exe）
    -CMake      cmake.exe 路径    （默认 D:\MyDesktop\tools\cmake\cmake-3.31.11-windows-x86_64\bin\cmake.exe）
    -BuildDir   构建目录          （默认 build-release）
    -DistDir    输出目录          （默认 dist\wter's tool）
    -BundleTools 连工具目录一起打包（自包含便携包）
    -ToolsSource 工具源目录       （默认 D:\MyDesktop\tools\ctf\tools）
#>
param(
    [string]$QtDir    = "D:\QT\6.10.2\mingw_64",
    [string]$MingwBin = "D:\QT\Tools\mingw1310_64\bin",
    [string]$Ninja    = "D:\QT\Tools\Ninja\ninja.exe",
    [string]$CMake    = "D:\MyDesktop\tools\cmake\cmake-3.31.11-windows-x86_64\bin\cmake.exe",
    [string]$BuildDir = "build-release",
    [string]$DistDir  = "dist\wter's tool",
    [switch]$BundleTools,
    [string]$ToolsSource = "D:\MyDesktop\tools\ctf\tools"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

function Assert-Exists([string]$Path, [string]$What) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "找不到$What：$Path" }
}

$Gxx        = Join-Path $MingwBin "g++.exe"
$Windeploy  = Join-Path $QtDir "bin\windeployqt.exe"

Assert-Exists $CMake       "cmake"
Assert-Exists $Ninja       "ninja"
Assert-Exists $Gxx         "g++（MinGW）"
Assert-Exists $Windeploy   "windeployqt"

# 工具链加入 PATH（cmake/ninja 构建与 windeployqt 都需要）
$env:PATH = "$MingwBin;$(Join-Path $QtDir 'bin');$(Split-Path -Parent $CMake);$(Split-Path -Parent $Ninja);$env:PATH"

Write-Host "==> 1/4 配置 CMake（Ninja + Release）" -ForegroundColor Cyan
& $CMake -S $Root -B (Join-Path $Root $BuildDir) -G Ninja `
    "-DCMAKE_BUILD_TYPE=Release" `
    "-DCMAKE_PREFIX_PATH=$QtDir" `
    "-DCMAKE_C_COMPILER=$(Join-Path $MingwBin 'gcc.exe')" `
    "-DCMAKE_CXX_COMPILER=$Gxx" `
    "-DCMAKE_MAKE_PROGRAM=$Ninja"
if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败" }

Write-Host "==> 2/4 编译" -ForegroundColor Cyan
& $CMake --build (Join-Path $Root $BuildDir)
if ($LASTEXITCODE -ne 0) { throw "编译失败" }

$SrcExe = Join-Path $Root "$BuildDir\wter's tool.exe"
Assert-Exists $SrcExe "编译产物 wter's tool.exe"

Write-Host "==> 3/4 生成发行目录 $DistDir" -ForegroundColor Cyan
$DistFull = Join-Path $Root $DistDir
if (Test-Path -LiteralPath $DistFull) { Remove-Item -LiteralPath $DistFull -Recurse -Force }
New-Item -ItemType Directory -Path $DistFull -Force | Out-Null
Copy-Item -LiteralPath $SrcExe -Destination $DistFull -Force

Write-Host "==> 4/4 收集 Qt / MinGW 运行库（windeployqt）" -ForegroundColor Cyan
& $Windeploy --release --no-translations --compiler-runtime --no-system-d3d-compiler `
    --dir $DistFull (Join-Path $DistFull "wter's tool.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败" }

foreach ($extra in @("LICENSE", "README.md")) {
    $p = Join-Path $Root $extra
    if (Test-Path -LiteralPath $p) { Copy-Item -LiteralPath $p -Destination $DistFull -Force }
}

if ($BundleTools) {
    Write-Host "==> 5/5 打包工具目录 → $DistDir\tools" -ForegroundColor Cyan
    Assert-Exists $ToolsSource "工具源目录"
    $ToolsDest = Join-Path $DistFull "tools"
    if (Test-Path -LiteralPath $ToolsDest) { Remove-Item -LiteralPath $ToolsDest -Recurse -Force }
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & robocopy $ToolsSource $ToolsDest /E /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
    $rc = $LASTEXITCODE
    $ErrorActionPreference = $prev
    if ($rc -ge 8) { throw "复制工具目录失败（robocopy exit=$rc）" }
    Write-Host "工具目录已打包：$ToolsDest" -ForegroundColor Green
}

Write-Host ""
Write-Host "完成：$DistFull" -ForegroundColor Green
Get-ChildItem -LiteralPath $DistFull | Select-Object Name, Length | Format-Table -AutoSize
