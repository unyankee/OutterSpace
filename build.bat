@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo    ToyEngine Build Utility
echo ==========================================

:: 1. Find MSBuild
set "MSBUILD_PATH="
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\Current\Bin\MSBuild.exe`) do (
    set "MSBUILD_PATH=%%i"
)

if not defined MSBUILD_PATH (
    echo [ERROR] MSBuild not found. Please ensure Visual Studio is installed with C++ development tools.
    pause
    exit /b 1
)
echo [INFO] Found MSBuild at: %MSBUILD_PATH%

:: 2. Build Solution
echo.
echo Building Solution (Debug x64)...
"%MSBUILD_PATH%" Engine\Engine.sln /p:Configuration=Debug /p:Platform=x64 /t:Build /m
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo    Build Successful!
echo ==========================================
echo Executable: Engine\x64\Debug\Engine.exe
echo.
pause
