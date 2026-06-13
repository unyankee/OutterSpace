@echo off
setlocal

:: This script runs GEnie to generate project files.
:: It defaults to vs2022 if no argument is provided.

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=vs2022"

if not exist "bin\genie.exe" (
    echo [ERROR] GEnie not found at bin\genie.exe.
    echo Please run setup.bat first.
    exit /b 1
)

echo [INFO] Generating %TARGET% project files...
bin\genie.exe %TARGET%

if %errorlevel% neq 0 (
    echo [ERROR] Project generation failed.
    exit /b 1
)

echo [INFO] Project files generated successfully in the 'build' directory.
pause
