@echo off
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-all.ps1"
if errorlevel 1 (
  echo.
  echo Build failed. Review the output and see docs\build-artifacts.md.
  pause
  exit /b 1
)
echo.
echo Build complete. See the dist folder for the portable package and SHA-256 file.
pause
endlocal
