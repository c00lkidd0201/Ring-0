@echo off
chcp 65001 >nul
echo ========================================
echo Roblox Executor DLL Build Script
 echo Windows 11 x64 | Release
 echo ========================================
echo.

set SOLUTION_DIR=%~dp0
set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build

if not exist "%VCINSTALLDIR%\vcvars64.bat" (
    echo Error: Visual Studio 2022 not found at: %VCINSTALLDIR%
    echo Please install Visual Studio 2022 with C++ desktop development
    pause
    exit /b 1
)

echo [1/3] Setting up environment...
call "%VCINSTALLDIR%\vcvars64.bat" >nul 2>&1

if errorlevel 1 (
    echo Error: Failed to set up Visual Studio environment
    pause
    exit /b 1
)

echo [2/3] Building Executor DLL...
cd /d "%SOLUTION_DIR%"

if exist "Ring0.sln" (
    msbuild Ring0.sln /p:Configuration=Release /p:Platform=x64 /p:BuildProject="Executor" /t:Build /nologo
) else (
    echo Error: Ring0.sln not found in %SOLUTION_DIR%
    pause
    exit /b 1
)

if errorlevel 1 (
    echo Error: Executor build failed
    pause
    exit /b 1
)

echo.
echo [3/3] Checking output...
if exist "Release\Executor\RobloxExecutor.dll" (
    echo Success! Executor DLL built at: Release\Executor\RobloxExecutor.dll
    echo.
    echo File info:
    for %%F in ("Release\Executor\RobloxExecutor.dll") do (
        echo   Size: %%~zF bytes
        echo   Created: %%~tF
    )
) else (
    echo Error: RobloxExecutor.dll not found in Release\Executor\
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
pause
