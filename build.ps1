param([string]$QtRoot = 'D:\Qt\Qt5.12.11')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$taskQtBin = Join-Path $QtRoot '5.12.11\mingw73_64\bin'
$taskCompilerBin = Join-Path $QtRoot 'Tools\mingw730_64\bin'
$taskQmake = Join-Path $taskQtBin 'qmake.exe'
$taskMake = Join-Path $taskCompilerBin 'mingw32-make.exe'
$taskOriginalPath = $env:PATH
$taskOriginalPlatform = $env:QT_QPA_PLATFORM
$taskOriginalFontDir = $env:QT_QPA_FONTDIR

function Invoke-QtBuild([string]$Directory, [string]$Project) {
    New-Item -ItemType Directory -Force -Path $Directory | Out-Null
    Push-Location $Directory
    try {
        & $taskQmake $Project
        if ($LASTEXITCODE -ne 0) { throw "qmake failed: $Project" }
        & $taskMake -j4
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $Project" }
    } finally {
        Pop-Location
    }
}

Push-Location $PSScriptRoot
try {
    $env:PATH = "$taskCompilerBin;$taskQtBin;$taskOriginalPath"
    Invoke-QtBuild 'build' '..\TextEditor.pro'
    Invoke-QtBuild 'build\tests' '..\..\tests\tests.pro'
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_QPA_FONTDIR = Join-Path $env:SystemRoot 'Fonts'
    & '.\build\tests\release\editor_tests.exe' -o 'build\tests\results.txt,txt'
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed; see build\tests\results.txt' }
    Get-Content -LiteralPath 'build\tests\results.txt'
    & (Join-Path $taskQtBin 'windeployqt.exe') --release --no-translations --no-system-d3d-compiler --no-opengl-sw 'build\release\TextEditor.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed' }
    New-Item -ItemType Directory -Force -Path 'build\release\translations' | Out-Null
    Copy-Item -LiteralPath (Join-Path $QtRoot '5.12.11\mingw73_64\translations\qtbase_zh_CN.qm') -Destination 'build\release\translations'
    Write-Output 'Ready: build\release\TextEditor.exe'
} finally {
    $env:PATH = $taskOriginalPath
    $env:QT_QPA_PLATFORM = $taskOriginalPlatform
    $env:QT_QPA_FONTDIR = $taskOriginalFontDir
    Pop-Location
}
