@echo off
setlocal EnableExtensions

set "SERVER_IP=%~1"
if not defined SERVER_IP (
  set /p "SERVER_IP=Enter the KMTGuard public server IP: "
)
if not defined SERVER_IP (
  echo Server IP is required.
  pause
  exit /b 1
)

echo ================================================================
echo  KMTGuard External Port Test
echo ================================================================
echo Server: %SERVER_IP%
echo.
echo Run this file from a DIFFERENT PC, not from the game server.
echo.

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ip='%SERVER_IP%';" ^
  "$ports=@(10001,10002,10003);" ^
  "foreach($p in $ports){" ^
  "  $r=Test-NetConnection -ComputerName $ip -Port $p -InformationLevel Quiet -WarningAction SilentlyContinue;" ^
  "  if($r){ Write-Host ('OPEN   {0}:{1}' -f $ip,$p) -ForegroundColor Green }" ^
  "  else{ Write-Host ('CLOSED {0}:{1}' -f $ip,$p) -ForegroundColor Red }" ^
  "}"

echo.
echo Expected:
echo  10002 = Filter Gateway  - launcher connects here
echo  10001 = Filter Download - patch/start flow may need this
echo  10003 = Filter Agent    - login redirect needs this
echo.
echo Keep the real Silkroad ports private. Only the selected filter ports
echo should be exposed to players.
echo.
pause
