@echo off
setlocal enabledelayedexpansion

:: Path to your compiler
set "COMPILER=glslangValidator.exe"
:: Track failed files in a list
set "FAILED_LIST="

:menu
echo.
echo ==========================================
echo    SHADER CROSS-FOLDER COMPILATOR
echo ==========================================
echo  1. Run/Re-run all shaders
echo  2. Re-run only failed shaders
echo  3. Clear console window
echo  4. Close
echo ==========================================
set /p user_choice="Select an option (1-4): "

if "%user_choice%"=="1" goto :compile_all
if "%user_choice%"=="2" goto :compile_failed
if "%user_choice%"=="3" (
    cls
    goto :menu
)
if "%user_choice%"=="4" exit
goto :menu

:compile_all
echo Starting full scan...
set "FAILED_LIST="
:: Scans for any .glsl file recursively
for /r %%f in (*.glsl) do (
    call :process_file "%%f"
)
echo.
echo Full scan finished.
goto :menu

:compile_failed
if "!FAILED_LIST!"=="" (
    echo No failed shaders to re-run.
    goto :menu
)
echo Re-running failed shaders...
set "CURRENT_FAILED=!FAILED_LIST!"
set "FAILED_LIST="
for %%f in (!CURRENT_FAILED!) do (
    call :process_file %%f
)
echo.
echo Retry pass finished.
goto :menu

:process_file
set "FULL_PATH=%~1"
set "FILE_NAME=%~nx1"
set "FINAL_NAME=%~n1.spv"
set "STYPE=Unknown Stage"

:: Check for shader stage keywords within the filename
echo !FILE_NAME! | findstr /i ".vert" >nul && set "STYPE=Vertex Shader"
echo !FILE_NAME! | findstr /i ".frag" >nul && set "STYPE=Fragment Shader"
echo !FILE_NAME! | findstr /i ".mesh" >nul && set "STYPE=Mesh Shader"
echo !FILE_NAME! | findstr /i ".task" >nul && set "STYPE=Task Shader"
echo !FILE_NAME! | findstr /i ".geom" >nul && set "STYPE=Geometry Shader"
echo !FILE_NAME! | findstr /i ".tesc" >nul && set "STYPE=Tessellation Control"
echo !FILE_NAME! | findstr /i ".tese" >nul && set "STYPE=Tessellation Evaluation"
echo !FILE_NAME! | findstr /i ".comp" >nul && set "STYPE=Compute Shader"

echo Found !FILE_NAME! -^> Compiled as !STYPE! -^> !FINAL_NAME!

:: Execute compilation using glslangValidator
"%COMPILER%" -V "%FULL_PATH%" -o "%~dpn1.spv" > compile_log.tmp 2>&1

if !errorlevel! equ 0 (
    echo [SUCCESS]
) else (
    echo [FAILURE]
    type compile_log.tmp
    set "FAILED_LIST=!FAILED_LIST! "%FULL_PATH%""
)
del compile_log.tmp
echo ------------------------------------------
goto :eof