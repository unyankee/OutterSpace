@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "VULKAN_VERSION=1.4.313.0"
set "VULKAN_PATH=C:\VulkanSDK\%VULKAN_VERSION%"

echo ==========================================
echo    ToyEngine Setup Utility
echo ==========================================

:: 0. Download GEnie
echo [0/7] Downloading GEnie...
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
echo [1/7] Initializing submodules...
git submodule update --init --recursive
if %errorlevel% neq 0 (
    echo [WARNING] Failed to initialize submodules. Ensure git is installed and you have internet access.
)

:: 1.5. Download STB
echo [1.5/7] Downloading STB...
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

:: 1.6. Download EnTT
echo [1.6/7] Downloading EnTT...
if not exist "extern\entt" (
    git clone --depth 1 https://github.com/skypjack/entt.git extern\entt
    if %errorlevel% neq 0 (
        echo [WARNING] Failed to clone EnTT. You might need to download it manually.
    ) else (
        echo [INFO] EnTT cloned successfully.
    )
) else (
    echo [INFO] EnTT already exists in extern/entt
)

:: 1.7. Download GLM
echo [1.7/7] Downloading GLM...
if not exist "extern\glm" (
    git clone --depth 1 https://github.com/g-truc/glm.git extern\glm
    if %errorlevel% neq 0 (
        echo [WARNING] Failed to clone glm. You might need to download it manually.
    ) else (
        echo [INFO] glm cloned successfully.
    )
) else (
    echo [INFO] glm already exists in extern/glm
)

:: 1.8. Download/convert volk
echo [1.8/7] Setting up volk...
if not exist "extern\volk\.git" (
    if exist "extern\volk" (
        echo [INFO] volk exists but was manually downloaded. Converting to git repo...
        rmdir /s /q extern\volk
    )
    git clone --depth 1 https://github.com/zeux/volk.git extern\volk
    if %errorlevel% neq 0 (
        echo [WARNING] Failed to clone volk. You might need to download it manually.
    ) else (
        echo [INFO] volk cloned successfully.
    )
) else (
    echo [INFO] volk already exists in extern/volk
)

:: 2. Update extern repos
echo [2/7] Updating extern repositories...
for %%D in (stb entt glm volk) do (
    if exist "extern\%%D\.git" (
        echo [INFO] Updating extern/%%D...
        pushd extern\%%D
        git fetch --depth 1 origin
        git reset --hard origin/HEAD
        if !errorlevel! neq 0 (
            echo [WARNING] Failed to update extern/%%D
        ) else (
            echo [INFO] extern/%%D updated successfully.
        )
        popd
    ) else (
        echo [WARNING] extern/%%D is not a git repo, skipping update.
    )
)
git submodule update --remote --recursive
if %errorlevel% neq 0 (
    echo [WARNING] Failed to update submodules to latest.
)

:: 3. Find/Verify Vulkan SDK
echo [3/7] Verifying Vulkan SDK...
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

:: 4. Initial Shader Compilation
echo [4/7] Compiling Shaders...
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