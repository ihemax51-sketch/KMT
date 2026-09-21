@echo off
setlocal EnableExtensions
if "%~1"=="" exit /b 2
if not exist "%~1" exit /b 2
call "C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /W4 /Fo"%~1\MagicPopResult.Tests.obj" /Fe"%~1\MagicPopResult.Tests.exe" "%~dp0MagicPopResult.Tests.cpp"
if errorlevel 1 exit /b %errorlevel%
"%~1\MagicPopResult.Tests.exe"
exit /b %errorlevel%
