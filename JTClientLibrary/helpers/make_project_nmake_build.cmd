@echo off
setlocal enableextensions

REM اسم ملف اللوج
set "LOG=build.log"

REM 1) تهيئة بيئة فيجوال ستوديو
call "%VS80COMNTOOLS%vsvars32.bat"
if errorlevel 1 (
  echo [ERROR] Failed to load VS environment. > "%LOG%"
  echo Check the VS80COMNTOOLS variable or your VS installation. >> "%LOG%"
  echo.
  echo Failed to load Visual Studio environment. See "%LOG%".
  echo.
  pause
  exit /b 1
)

REM 2) تشغيل البناء مع توجيه الخرج (stdout+stderr) للّوج
echo ==== Build started %date% %time% ==== > "%LOG%"
cmake --build . --config RelWithDebInfo >> "%LOG%" 2>&1
set "RC=%errorlevel%"

if not "%RC%"=="0" (
  echo.>> "%LOG%"
  echo ==== BUILD FAILED (errorlevel %RC%) ====>> "%LOG%"
  echo.
  echo Build failed with errorlevel %RC%.
  echo Showing log (saved at: %CD%\%LOG%):
  echo ------------------------------------------------------------
  type "%LOG%"
  echo ------------------------------------------------------------
  echo.
  echo Press any key to close...
  pause >nul
  exit /b %RC%
) else (
  echo ==== BUILD SUCCEEDED ====>> "%LOG%"
  echo Build succeeded. Log saved at: %CD%\%LOG%
  echo.
  echo Press any key to close...
  pause >nul
  exit /b 0
)
