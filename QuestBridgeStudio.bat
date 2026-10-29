@echo off
setlocal
cd /d "%~dp0"
if exist "QuestBridgeStudio.exe" (
  start "QuestBridge Studio" "QuestBridgeStudio.exe"
  exit /b 0
)
where py >nul 2>nul
if %errorlevel%==0 (
  py -3 tools\questbridge_studio.py
) else (
  python tools\questbridge_studio.py
)
if errorlevel 1 pause
endlocal
