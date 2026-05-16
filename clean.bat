@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo    ToyEngine Cleanup Utility
echo ==========================================
echo This will remove all generated build artifacts and temporary files.
set /p confirm="Are you sure you want to proceed? (Y/N): "
if /i "%confirm%" neq "Y" (
    echo Cleanup cancelled.
    exit /b 0
)

echo.
echo Cleaning up build artifacts...

:: 1. Remove Visual Studio temporary files
if exist ".vs" (
    echo   Removing .vs/
    rd /s /q ".vs"
)

:: 2. Remove Engine build folders
pushd Engine
for %%d in (x64 x86 Debug Release) do (
    if exist "%%d" (
        echo   Removing Engine/%%d/
        rd /s /q "%%d"
    )
)

:: 3. Remove Project-specific build folders
for /d /r %%d in (x64 x86 Debug Release) do (
    if exist "%%d" (
        :: Avoid deleting assets or extern folders if they happen to match these names
        echo %%d | findstr /i "extern assets" >nul
        if !errorlevel! neq 0 (
            echo   Removing %%d
            rd /s /q "%%d"
        )
    )
)
popd

:: 4. Remove Compiled Shaders
echo.
echo Cleaning up compiled shaders...
pushd Engine\Shaders
for %%f in (*.spv) do (
    echo   Deleting %%f
    del "%%f"
)
popd

:: 5. Remove temporary log files
if exist "Engine\Shaders\compile_log.tmp" del "Engine\Shaders\compile_log.tmp"

echo.
echo ==========================================
echo    Cleanup Complete!
echo ==========================================
echo Project is now in a "bare minimum" state.
echo Run setup.bat to re-initialize the environment.
echo.
pause
