@echo off

call "%VS80COMNTOOLS%vsvars32.bat"

cmake --build . --config RelWithDebInfo

rem خزن كود الخروج من آخر أمر
set "rc=%ERRORLEVEL%"

echo.
if %rc% neq 0 (
    echo ===== BUILD FAILED (exit code %rc%) =====
) else (
    echo ===== BUILD SUCCEEDED =====
)

echo.
echo Press any key to close...
pause >nul

exit /b %rc%