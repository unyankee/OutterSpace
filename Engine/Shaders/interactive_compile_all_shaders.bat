@echo off
setlocal enabledelayedexpansion

:: Path to your compiler
set "COMPILER=glslangValidator.exe"

:menu
echo.
echo ==========================================
echo    INTERACTIVE SHADER COMPILATOR
echo ==========================================
echo  1. Run/Re-run all shaders
echo  2. Clear console window
echo  3. Close
echo ==========================================
set /p user_choice="Select an option (1-3): "

if "%user_choice%"=="1" (
    call compile_all_shaders.bat
    goto :menu
)
if "%user_choice%"=="2" (
    cls
    goto :menu
)
if "%user_choice%"=="3" exit
goto :menu
