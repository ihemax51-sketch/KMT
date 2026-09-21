@echo off
setlocal
cd /d "%~dp0"

set "DESKTOP_EXE=D:\KMTGuard-build\Filter\KMTGuard.exe"

if not exist "%DESKTOP_EXE%" (
    echo The official desktop build is missing. Building the official release now...
    set "NO_PAUSE=1"
    call "04_BUILD_FILTER_FULL_REBUILD.cmd"
    if errorlevel 1 exit /b %errorlevel%
)

start "" "%DESKTOP_EXE%"
