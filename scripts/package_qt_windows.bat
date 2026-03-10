@echo off
setlocal enabledelayedexpansion

REM Build and package Qt Windows installer (UI parity with macOS Qt version).
REM Prereqs:
REM 1) Qt5 installed on Windows and windeployqt in PATH.
REM 2) CMake + NSIS (makensis) installed.
REM 3) Run from "x64 Native Tools Command Prompt" if using MSVC generator.

set ROOT=%~dp0..
for %%I in ("%ROOT%") do set ROOT=%%~fI

set BUILD_DIR=%ROOT%\build-win-qt
set STAGE_DIR=%BUILD_DIR%\stage
set DIST_DIR=%ROOT%\dist
set NSI_FILE=%ROOT%\packaging\qt_installer.nsi
set OUT_FILE=%DIST_DIR%\PhoneFinder-installer.exe

if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%STAGE_DIR%" mkdir "%STAGE_DIR%"

echo [1/4] Configure CMake...
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1

echo [2/4] Build PhoneFinder...
cmake --build "%BUILD_DIR%" --config Release --target phonefinder_qt
if errorlevel 1 exit /b 1

echo [3/4] Stage files and deploy Qt runtime...
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%STAGE_DIR%"
copy /y "%BUILD_DIR%\Release\PhoneFinder.exe" "%STAGE_DIR%\"
copy /y "%ROOT%\samples\医院科室电话数据模板_demo.csv" "%STAGE_DIR%\"
if exist "%ROOT%\resources\windows\icon.ico" copy /y "%ROOT%\resources\windows\icon.ico" "%STAGE_DIR%\icon.ico"
if exist "%ROOT%\resources\trayicon.png" copy /y "%ROOT%\resources\trayicon.png" "%STAGE_DIR%\trayicon.png"
windeployqt --release "%STAGE_DIR%\PhoneFinder.exe"
if errorlevel 1 exit /b 1

echo [4/4] Build installer...
makensis /DAPP_STAGE_DIR="%STAGE_DIR%" /DAPP_OUTFILE="%OUT_FILE%" "%NSI_FILE%"
if errorlevel 1 exit /b 1

echo.
echo Installer built: %OUT_FILE%
exit /b 0
