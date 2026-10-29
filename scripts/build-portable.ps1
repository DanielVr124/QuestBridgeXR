param(
    [switch]$SkipPythonTools,
    [switch]$SkipArchive
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\host'
$distRoot = Join-Path $root 'dist'
$stage = Join-Path $distRoot 'QuestBridgeXR-Portable'
$work = Join-Path $env:TEMP 'QuestBridgeXR-PyInstaller'

function Require-Command([string]$Name, [string]$Help) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name not found. $Help"
    }
}
Require-Command 'cmake' 'Install CMake 3.24+ and run from a VS 2022 Developer PowerShell.'
Require-Command 'python' 'Install Python 3.11+ or use the bundled Windows build workflow.'

Write-Host '[1/5] Building Windows UDP host with CMake/MSVC...' -ForegroundColor Cyan
cmake -S (Join-Path $root 'host') -B $build -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed. Use VS 2022 Developer PowerShell with Desktop development with C++ installed.' }
cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw 'Windows host compilation failed; review the CMake/MSVC output above.' }
$hostExe = Join-Path $build 'Release\QuestBridgeHost.exe'
if (-not (Test-Path $hostExe)) { throw "CMake completed but host executable is missing: $hostExe" }

Write-Host '[2/6] Building experimental Windows OpenXR runtime/compositor (D3D11)...' -ForegroundColor Cyan
$runtimeBuild = Join-Path $root 'build\runtime'
cmake -S (Join-Path $root 'runtime') -B $runtimeBuild -A x64
if ($LASTEXITCODE -ne 0) { throw 'OpenXR runtime CMake configuration failed. CMake will fetch the pinned Khronos OpenXR-SDK; check network access and VS 2022 C++ components.' }
cmake --build $runtimeBuild --config Release
if ($LASTEXITCODE -ne 0) { throw 'Experimental OpenXR runtime compilation failed; review the CMake/MSVC output above.' }
$runtimeDll = Join-Path $runtimeBuild 'Release\qbx_openxr_runtime.dll'
if (-not (Test-Path $runtimeDll)) { throw "CMake completed but runtime DLL is missing: $runtimeDll" }

Write-Host '[3/6] Staging portable application files...' -ForegroundColor Cyan
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
$include = @('README.md','LICENSE','CHANGELOG.md','QuestBridgeStudio.bat','Build-QuestBridgeXR.bat','Install-QuestClient.bat','config','docs','host','protocol','quest-client','runtime','scripts','tools')
foreach ($item in $include) {
    $source = Join-Path $root $item
    if (Test-Path $source) {
        Copy-Item $source -Destination $stage -Recurse -Force
    }
}
New-Item -ItemType Directory -Path (Join-Path $stage 'host') -Force | Out-Null
Copy-Item $hostExe (Join-Path $stage 'host\QuestBridgeHost.exe') -Force
$stageRuntime = Join-Path $stage 'runtime'
New-Item -ItemType Directory -Path $stageRuntime -Force | Out-Null
Copy-Item $runtimeDll (Join-Path $stageRuntime 'qbx_openxr_runtime.dll') -Force
Copy-Item (Join-Path $root 'runtime\manifest\questbridge_openxr_runtime.json') (Join-Path $stageRuntime 'questbridge_openxr_runtime.json') -Force

if (-not $SkipPythonTools) {
    Write-Host '[4/6] Ensuring PyInstaller is installed...' -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $work -Force | Out-Null
    & python -m pip show pyinstaller *> $null
    if ($LASTEXITCODE -ne 0) { & python -m pip install --user pyinstaller }
    if ($LASTEXITCODE -ne 0) { throw 'Could not install PyInstaller. Check network access or install it manually.' }
    & python -m PyInstaller --version | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'PyInstaller is not available to this Python interpreter.' }
    $toolsOut = Join-Path $stage 'tools'
    $studioScript = Join-Path $root 'tools\questbridge_studio.py'
    & python -m PyInstaller --noconfirm --clean --onefile --windowed --name QuestBridgeStudio --distpath $stage --workpath (Join-Path $work 'studio') --specpath $work --paths (Join-Path $root 'tools') $studioScript
    if ($LASTEXITCODE -ne 0) { throw 'PyInstaller failed building QuestBridgeStudio.exe' }
    foreach ($tool in @('doctor.py','stream_test_video.py','monitor_telemetry.py','openxr_runtime.py')) {
        $name = [IO.Path]::GetFileNameWithoutExtension($tool)
        & python -m PyInstaller --noconfirm --clean --onefile --name $name --distpath $toolsOut --workpath (Join-Path $work $name) --specpath $work --paths (Join-Path $root 'tools') (Join-Path $root "tools\$tool")
        if ($LASTEXITCODE -ne 0) { throw "PyInstaller failed building $name.exe" }
    }
} else {
    Write-Host 'Skipping Python executable packaging as requested.' -ForegroundColor Yellow
}

Write-Host '[5/6] Writing build metadata...' -ForegroundColor Cyan
@"
QuestBridge XR portable Windows package
Built: $(Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
Windows host: host\QuestBridgeHost.exe
Studio: $(if (Test-Path (Join-Path $stage 'QuestBridgeStudio.exe')) { 'QuestBridgeStudio.exe' } else { 'Python source: tools\questbridge_studio.py' })
Experimental native OpenXR runtime: runtime\qbx_openxr_runtime.dll (D3D11; not yet conformance-tested). Select only per application via Studio.
Compositor preview capture: python tools\stream_test_video.py --source mirror --layout sbs (requires FFmpeg).
Meta Quest Link remains the fallback for compatible applications; do not set QuestBridge as system-wide active runtime.
Quest app: build quest-client\ with Android Studio/Gradle, or download the separate APK build artifact from CI.
"@ | Set-Content -Encoding UTF8 (Join-Path $stage 'BUILD-INFO.txt')

if (-not $SkipArchive) {
    Write-Host '[6/6] Creating portable ZIP...' -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
    $zip = Join-Path $distRoot 'QuestBridgeXR-Windows-Portable.zip'
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
    Write-Host "Portable package written: $zip" -ForegroundColor Green
} else {
    Write-Host '[6/6] Skipping archive as requested.' -ForegroundColor Yellow
}
