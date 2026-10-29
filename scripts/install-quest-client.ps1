$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$adb = Get-Command adb -ErrorAction SilentlyContinue
if (-not $adb) {
    throw 'adb was not found. Install Android SDK Platform-Tools, add adb to PATH, enable Quest Developer Mode and USB debugging, then reconnect the headset.'
}
$apkCandidates = @(
    (Join-Path $root 'QuestClient\QuestBridgeXR-Quest-debug.apk'),
    (Join-Path $root 'quest-client\app\build\outputs\apk\debug\app-debug.apk')
)
$apk = $apkCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $apk) {
    throw 'Quest APK not found. Use the combined QuestBridgeXR portable ZIP from the build workflow or build quest-client with Android Studio/Gradle.'
}
$devicesOutput = & adb devices
$online = @($devicesOutput | Select-Object -Skip 1 | Where-Object { $_ -match '^\S+\s+device$' } | ForEach-Object { ($_ -split '\s+')[0] })
if ($online.Count -ne 1) {
    throw "Expected exactly one authorized Quest/Android device, found $($online.Count). Run 'adb devices' and accept the USB-debugging prompt on the headset."
}
& adb -s $online[0] install -r $apk
if ($LASTEXITCODE -ne 0) { throw 'adb install failed.' }
& adb -s $online[0] shell monkey -p com.questbridge.xr 1
if ($LASTEXITCODE -ne 0) { throw 'APK installed, but Android could not launch it automatically.' }
Write-Host 'QuestBridge XR was installed and launch was requested.' -ForegroundColor Green
