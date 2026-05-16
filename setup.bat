@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "VULKAN_VERSION=1.4.313.0"
set "VULKAN_PATH=C:\VulkanSDK\%VULKAN_VERSION%"

echo ==========================================
echo    ToyEngine Setup Utility
echo ==========================================

:: 1. Initialize Submodules
echo [1/3] Initializing submodules...
git submodule update --init --recursive
if %errorlevel% neq 0 (
    echo [WARNING] Failed to initialize submodules. Ensure git is installed and you have internet access.
)

:: 2. Find/Verify Vulkan SDK
echo [2/3] Verifying Vulkan SDK...
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
echo [3/3] Compiling Shaders...
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
