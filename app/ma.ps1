param (
    [switch]$clean
)

$ErrorActionPreference = "Stop"

# MSYS2 UCRT64 の bin を PATH の最優先に追加（cc1plus.exe の DLL 参照不整合を防止）
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

$AppDir = $PSScriptRoot
Set-Location $AppDir

# 1. -clean オプション処理
if ($clean) {
    Write-Host "[clean] Removing build directory..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
    }
}

# 2. 必要なディレクトリ準備
$BuildDir = Join-Path $AppDir "build"
$ObjDir = Join-Path $BuildDir "obj"
$SingleIncludeDir = Join-Path $AppDir "single_include"
$JsonHeader = Join-Path $SingleIncludeDir "json.hpp"
$ImguiDir = Join-Path $AppDir "imgui"

if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
if (-not (Test-Path $ObjDir)) { New-Item -ItemType Directory -Path $ObjDir | Out-Null }
if (-not (Test-Path $SingleIncludeDir)) { New-Item -ItemType Directory -Path $SingleIncludeDir | Out-Null }

# json.hpp の自動取得
if (-not (Test-Path $JsonHeader)) {
    Write-Host "[deps] Fetching json.hpp..." -ForegroundColor Cyan
    curl.exe -Lo $JsonHeader https://github.com/nlohmann/json/releases/latest/download/json.hpp
}

# ImGui の自動クローン
if (-not (Test-Path $ImguiDir)) {
    Write-Host "[deps] Cloning ImGui..." -ForegroundColor Cyan
    git clone --depth 1 https://github.com/ocornut/imgui.git -b v1.90.8 $ImguiDir
}

# 3. コンパイル設定
$Gxx = "C:/msys64/ucrt64/bin/g++.exe"
$OutExe = Join-Path $BuildDir "istack_push.exe"

$Sources = @(
    "main.cpp",
    "imgui/imgui.cpp",
    "imgui/imgui_draw.cpp",
    "imgui/imgui_tables.cpp",
    "imgui/imgui_widgets.cpp",
    "imgui/backends/imgui_impl_glfw.cpp",
    "imgui/backends/imgui_impl_opengl3.cpp"
)

$Includes = @(
    "-std=c++17",
    "-O2",
    "-I.",
    "-Iimgui",
    "-Iimgui/backends",
    "-Isingle_include",
    "-IC:/msys64/ucrt64/include"
)

$ObjectFiles = @()
$Total = $Sources.Count
$Current = 0

Write-Host "[build] Compiling source files..." -ForegroundColor Green

# 各ファイルを個別コンパイルして進捗を表示
foreach ($src in $Sources) {
    $Current++
    $percent = [math]::Round(($Current / $Total) * 100)
    $objName = ([System.IO.Path]::GetFileNameWithoutExtension($src)) + ".o"
    $objPath = Join-Path $ObjDir $objName
    $ObjectFiles += $objPath

    Write-Host ("[{0}/{1}] ({2}%) Compiling {3}..." -f $Current, $Total, $percent, $src) -ForegroundColor Cyan

    $CompileArgs = $Includes + @("-c", $src, "-o", $objPath)
    & $Gxx @CompileArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Host "[error] Failed to compile $src" -ForegroundColor Red
        exit 1
    }
}

# リンク処理
Write-Host "[link] Linking $OutExe..." -ForegroundColor Green
$LinkArgs = @("-LC:/msys64/ucrt64/lib") + $ObjectFiles + @(
    "-lglfw3",
    "-lopengl32",
    "-lgdi32",
    "-limm32",
    "-mwindows",
    "-o", $OutExe
)

& $Gxx @LinkArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host "[success] Built successfully: $OutExe" -ForegroundColor Green
} else {
    Write-Host "[error] Link failed." -ForegroundColor Red
}