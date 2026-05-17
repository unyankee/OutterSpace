@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "VULKAN_VERSION=1.4.313.0"
set "VULKAN_PATH=C:\VulkanSDK\%VULKAN_VERSION%"

echo ==========================================
echo    ToyEngine Setup Utility
echo ==========================================

:: 0. Download GEnie
echo [0/5] Downloading GEnie...
if not exist "bin" mkdir bin
if not exist "bin\genie.exe" (
    curl -L -o bin\genie.exe https://github.com/bkaradzic/bx/raw/master/tools/bin/windows/genie.exe
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to download GEnie.
        pause
        exit /b 1
    )
    echo [INFO] GEnie downloaded successfully.
) else (
    echo [INFO] GEnie already exists in bin/
)

:: 1. Initialize Submodules
echo [1/5] Initializing submodules...
git submodule update --init --recursive
if %errorlevel% neq 0 (
    echo [WARNING] Failed to initialize submodules. Ensure git is installed and you have internet access.
)

:: 1.5. Download STB
echo [1.5/5] Downloading STB...
if not exist "extern\stb" (
    git clone --depth 1 https://github.com/nothings/stb.git extern\stb
    if %errorlevel% neq 0 (
        echo [WARNING] Failed to clone STB. You might need to download it manually.
    ) else (
        echo [INFO] STB cloned successfully.
    )
) else (
    echo [INFO] STB already exists in extern/stb
)

:: 2. Find/Verify Vulkan SDK
echo [2/5] Verifying Vulkan SDK...
if not defined VULKAN_SDK (
    if exist "%VULKAN_PATH%" (
        set "VULKAN_SDK=%VULKAN_PATH%"
        echo [INFO] Found Vulkan SDK at %VULKAN_SDK%
    ) else (
        echo [ERROR] VULKAN_SDK environment variable not set and %VULKAN_PATH% not found.
        echo Please install Vulkan SDK version %VULKAN_VERSION% or set VULKAN_SDK.
        pause
        exit /b 1
    )
) else (
    echo [INFO] Using VULKAN_SDK: %VULKAN_SDK%
)

:: 3. Initial Shader Compilation
echo [3/5] Compiling Shaders...
if not exist "Engine\Shaders" (
    echo [ERROR] Engine\Shaders directory not found.
    pause
    exit /b 1
)

pushd Engine\Shaders
set "COMPILER=%VULKAN_SDK%\Bin\glslangValidator.exe"
if not exist "!COMPILER!" (
    echo [ERROR] Shader compiler not found at !COMPILER!
    popd
    pause
    exit /b 1
)
call compile_all_shaders.bat
popd

echo.
echo ==========================================
echo    Setup Complete!
echo ==========================================
echo All dependencies and shaders are ready.
echo Use build.bat to compile the engine.
echo.
pause
