@echo off
chcp 65001 >nul
echo ========================================
echo Ring-0 Driver Build Script
 echo Windows 11 x64 | Release | No WDK
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

echo [2/3] Building Driver...
cd /d "%SOLUTION_DIR%"

if exist "Ring0.sln" (
    msbuild Ring0.sln /p:Configuration=Release /p:Platform=x64 /p:BuildProject="Driver" /t:Build /nologo
) else (
    echo Error: Ring0.sln not found in %SOLUTION_DIR%
    pause
    exit /b 1
)

if errorlevel 1 (
    echo Error: Driver build failed
    pause
    exit /b 1
)

echo.
echo [3/3] Checking output...
if exist "Release\Driver\Ring0Driver.sys" (
    echo Success! Driver built at: Release\Driver\Ring0Driver.sys
    echo.
    echo File info:
    for %%F in ("Release\Driver\Ring0Driver.sys") do (
        echo   Size: %%~zF bytes
        echo   Created: %%~tF
    )
) else (
    echo Error: Ring0Driver.sys not found in Release\Driver\
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
pause
