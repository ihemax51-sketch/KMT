@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Silkroad Server Ports

echo ============================================================
echo        Silkroad Agent / Gateway / Download Ports
echo ============================================================
echo.

call :ShowPorts "AgentServer.exe" "AGENT"
call :ShowPorts "GatewayServer.exe" "GATEWAY"
call :ShowPorts "DownloadServer.exe" "DOWNLOAD"

echo ============================================================
echo Finished. Only TCP ports in LISTENING state are shown.
echo If a module is missing, start it and run this file again.
echo ============================================================
echo.
pause
exit /b 0

:ShowPorts
set "ImageName=%~1"
set "DisplayName=%~2"
set "ProcessFound=0"
set "PortFound=0"

echo [%DisplayName%]

for /f "usebackq tokens=1,2 delims=," %%A in (`tasklist /FI "IMAGENAME eq %ImageName%" /FO CSV /NH 2^>nul`) do (
    set "CurrentImage=%%~A"
    set "CurrentPid=%%~B"

    if /I "!CurrentImage!"=="%ImageName%" (
        set "ProcessFound=1"

        for /f "tokens=2,4,5" %%L in ('netstat -ano -p TCP 2^>nul ^| findstr /R /C:"LISTENING[ ]*!CurrentPid!$"') do (
            set "PortFound=1"
            set "LocalEndpoint=%%L"
            set "LocalPort=%%L"

            rem Keep only the value after the final colon (works with IPv4 and IPv6).
            for %%P in (!LocalPort::= !) do set "LocalPort=%%P"

            echo   PID !CurrentPid!  ^|  Port !LocalPort!  ^|  Listen: !LocalEndpoint!
        )
    )
)

if "!ProcessFound!"=="0" (
    echo   NOT RUNNING - %ImageName% was not found.
) else if "!PortFound!"=="0" (
    echo   RUNNING, but no TCP LISTENING port was found.
)

echo.
exit /b 0
