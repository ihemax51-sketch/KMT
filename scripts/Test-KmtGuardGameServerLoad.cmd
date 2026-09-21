@echo off
setlocal EnableExtensions

set "SERVER_ROOT=%~dp0..\gameserver"
set "BUILD_DIR=%SERVER_ROOT%\cmake-build-relwithdebinfo"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=C:\Program Files (x86)\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" exit /b 5

set "VS2005_VARS=C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
if not exist "%VS2005_VARS%" exit /b 3
call "%VS2005_VARS%"
if errorlevel 1 exit /b %errorlevel%

"%CMAKE_EXE%" --build "%BUILD_DIR%" --target KmtGuardGameServerLoadTest
if errorlevel 1 exit /b %errorlevel%

"%BUILD_DIR%\load-test\KmtGuardGameServerLoadTest.exe"
exit /b %errorlevel%
