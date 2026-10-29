$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build'

cmake -S (Join-Path $root 'host') -B (Join-Path $build 'host') -A x64
cmake --build (Join-Path $build 'host') --config Release
Write-Host "`nBuild complete. Run: $build\host\Release\QuestBridgeHost.exe"
