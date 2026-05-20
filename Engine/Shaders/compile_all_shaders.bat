@echo off
setlocal enabledelayedexpansion

:: Path to your compiler - default to PATH if not specified
if not defined COMPILER set "COMPILER=glslangValidator.exe"

echo Starting full scan...
:: Scans for any .glsl file recursively
for /r %%f in (*.glsl) do (
    call :process_file "%%f"
)
echo.
echo Full scan finished.
goto :eof

:process_file
set "FULL_PATH=%~1"
set "FILE_NAME=%~nx1"
set "FINAL_NAME=%~n1.spv"
set "STYPE=Unknown Stage"

if /i "!FILE_NAME!"=="common.glsl" goto :eof

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
"%COMPILER%" -V --target-env vulkan1.3 "%FULL_PATH%" -o "%~dpn1.spv" > compile_log.tmp 2>&1

if !errorlevel! equ 0 (
    echo [SUCCESS]
) else (
    echo [FAILURE]
    type compile_log.tmp
)
del compile_log.tmp
echo ------------------------------------------
goto :eof
