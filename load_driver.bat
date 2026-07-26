@echo off
chcp 65001 >nul
echo ========================================
echo Ring-0 Driver Loader
 echo Using kdmapper_Release.exe
 echo ========================================
echo.

set DRIVER_PATH=%~dp0Release\Driver\Ring0Driver.sys
set KDMAPPER_PATH=%~dp0kdmapper_Release.exe

echo Checking files...
if not exist "%DRIVER_PATH%" (
    echo Error: Driver not found at: %DRIVER_PATH%
    echo Please build the driver first
    pause
    exit /b 1
)

if not exist "%KDMAPPER_PATH%" (
    echo Error: kdmapper_Release.exe not found at: %KDMAPPER_PATH%
    echo Please download kdmapper and place it in the project directory
    pause
    exit /b 1
)

echo Driver file: %DRIVER_PATH%
echo kdmapper: %KDMAPPER_PATH%
echo.

net session >nul 2>&1
if errorlevel 1 (
    echo Error: Administrator privileges required
    echo Please run this script as Administrator
    pause
    exit /b 1
)

echo Loading driver with kdmapper...
echo.
"%KDMAPPER_PATH%" "%DRIVER_PATH%"

if errorlevel 1 (
    echo.
    echo Error: Failed to load driver
    echo Check DebugView for more information
    pause
    exit /b 1
)

echo.
echo ========================================
echo Driver loaded successfully!
echo ========================================
echo.
echo You can now use the Executor DLL to interact with the driver
echo.
echo To test the driver connection:
 echo   1. Run DebugView as Administrator
 echo   2. Run a test application that calls TestDriverConnection()
 echo   3. Check DebugView for [Ring0] messages
 echo.
pause
