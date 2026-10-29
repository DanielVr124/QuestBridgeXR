param(
    [switch]$SkipQuestBuild,
    [switch]$SkipArchive,
    [string]$QuestApkPath = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist 'QuestBridgeXR-Portable'
$version = '0.8.1'

if ($env:OS -ne 'Windows_NT') {
    throw 'This combined local build script must run on Windows. Use the GitHub Actions workflow to build Windows and Quest artifacts on hosted runners.'
}

Write-Host '[1/4] Build the standalone Windows host and Studio...' -ForegroundColor Cyan
& (Join-Path $PSScriptRoot 'build-portable.ps1') -SkipArchive
if ($LASTEXITCODE -ne 0) { throw 'Windows portable build failed.' }

$apk = $null
if ($QuestApkPath -and (Test-Path $QuestApkPath)) {
    $apk = (Resolve-Path $QuestApkPath).Path
} elseif (-not $SkipQuestBuild) {
    $gradle = Get-Command 'gradle' -ErrorAction SilentlyContinue
    $sdk = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { $env:ANDROID_HOME }
    if ($gradle -and $sdk) {
        Write-Host '[2/4] Building the Quest APK with Gradle/Android SDK...' -ForegroundColor Cyan
        $ndk = Join-Path $sdk 'ndk\27.2.12479018'
        if (Test-Path $ndk) {
            $env:ANDROID_NDK_HOME = $ndk
            $env:ANDROID_NDK_ROOT = $ndk
        }
        & $gradle.Source '-p' (Join-Path $root 'quest-client') ':app:assembleDebug' '--no-daemon' '--stacktrace'
        if ($LASTEXITCODE -ne 0) { throw 'Quest APK build failed. See Gradle output above.' }
        $candidate = Join-Path $root 'quest-client\app\build\outputs\apk\debug\app-debug.apk'
        if (-not (Test-Path $candidate)) { throw "Gradle reported success but the Quest APK was not found at $candidate" }
        $apk = $candidate
    } else {
        Write-Warning 'Gradle or ANDROID_HOME/ANDROID_SDK_ROOT is missing. Skipping the Quest APK; the Windows package will still be created. See docs/build-artifacts.md.'
    }
}

if ($apk) {
    Write-Host '[3/4] Bundling Quest APK...' -ForegroundColor Cyan
    $questDir = Join-Path $stage 'QuestClient'
    New-Item -ItemType Directory -Path $questDir -Force | Out-Null
    Copy-Item $apk (Join-Path $questDir 'QuestBridgeXR-Quest-debug.apk') -Force
    $apkStatus = 'Included: QuestClient\QuestBridgeXR-Quest-debug.apk'
} else {
    $apkStatus = 'Not included: build with Android SDK/NDK and Gradle, or download the combined CI package.'
}

@"
QuestBridge XR $version portable package
Built: $(Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
Windows Studio: QuestBridgeStudio.exe
Windows host: host\QuestBridgeHost.exe
Quest APK: $apkStatus

IMPORTANT: Includes an experimental D3D11 OpenXR runtime and mirror compositor. It has not passed
Khronos conformance and may not run arbitrary OpenXR apps. Select only per process; do not register
system-wide. Meta Quest Link remains the fallback for compatible apps. See runtime\README.md.
"@ | Set-Content -Encoding UTF8 (Join-Path $stage 'BUILD-INFO.txt')

if (-not $SkipArchive) {
    Write-Host '[4/4] Creating portable ZIP and SHA-256 checksum...' -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $dist -Force | Out-Null
    $zip = Join-Path $dist "QuestBridgeXR-$version-Portable.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal -Force
    $hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $(Split-Path -Leaf $zip)" | Set-Content -Encoding ASCII "$zip.sha256"
    Write-Host "Portable package: $zip" -ForegroundColor Green
    Write-Host "SHA-256: $hash" -ForegroundColor Green
} else {
    Write-Host 'Archive skipped.' -ForegroundColor Yellow
}
