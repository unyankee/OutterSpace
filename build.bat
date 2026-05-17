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

:: 2. Generate Project Files
echo.
echo Generating Project Files (vs2022)...
if not exist "bin\genie.exe" (
    echo [ERROR] GEnie not found. Please run setup.bat first.
    pause
    exit /b 1
)
bin\genie.exe vs2022
if %errorlevel% neq 0 (
    echo [ERROR] GEnie failed to generate project files.
    pause
    exit /b 1
)

:: 3. Build Solution
echo.
echo Building Solution (Debug x64)...
"%MSBUILD_PATH%" build\ToyEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:Build /m
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo    Build Successful!
echo ==========================================
echo Executable: bin\Debug\Engine.exe
echo.
pause
