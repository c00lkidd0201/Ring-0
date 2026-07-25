@echo off
:: ============================================
:: NO-WDK Driver Install/Uninstall Script
:: For Test Mode (x64 Release)
:: ============================================

:: Check if running as Administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] This script must be run as Administrator!
    pause
    exit /b 1
)

:: Set paths
set DRIVER_NAME=MemoryDriver
set DRIVER_FILE=Ring0Driver.sys
set SERVICE_NAME=MemoryDriverService

:: Check if driver file exists
if not exist "%~dp0x64\Release\%DRIVER_FILE%" (
    echo [ERROR] Driver file not found: %~dp0x64\Release\%DRIVER_FILE%
    echo Please build the project first (x64 Release).
    pause
    exit /b 1
)

:: ============================================
:: Menu
:: ============================================
:MENU
cls
echo.
echo ============================================
echo NO-WDK Driver Manager (Test Mode)
echo ============================================
echo 1. Install Driver
echo 2. Uninstall Driver
echo 3. Start Driver
echo 4. Stop Driver
echo 5. Exit
echo ============================================
echo.

set /p choice=Enter your choice (1-5):

if "%choice%"=="1" goto INSTALL
if "%choice%"=="2" goto UNINSTALL
if "%choice%"=="3" goto START
if "%choice%"=="4" goto STOP
if "%choice%"=="5" goto EXIT

echo Invalid choice. Please try again.
pause
goto MENU

:INSTALL
:: Install driver as a service
echo.
echo [INFO] Installing driver service...
sc create %SERVICE_NAME% binPath= "%~dp0x64\Release\%DRIVER_FILE%" type= kernel start= demand >nul 2>&1
if %errorLevel% equ 0 (
    echo [SUCCESS] Driver service installed.
) else (
    echo [ERROR] Failed to install driver service.
    echo Check if the driver is already installed or if Test Mode is enabled.
)
pause
goto MENU

:UNINSTALL
:: Uninstall driver service
echo.
echo [INFO] Stopping driver service...
sc stop %SERVICE_NAME% >nul 2>&1
echo [INFO] Removing driver service...
sc delete %SERVICE_NAME% >nul 2>&1
if %errorLevel% equ 0 (
    echo [SUCCESS] Driver service uninstalled.
) else (
    echo [ERROR] Failed to uninstall driver service.
)
pause
goto MENU

:START
:: Start driver service
echo.
echo [INFO] Starting driver service...
sc start %SERVICE_NAME% >nul 2>&1
if %errorLevel% equ 0 (
    echo [SUCCESS] Driver service started.
) else (
    echo [ERROR] Failed to start driver service.
    echo Check if Test Mode is enabled (bcdedit /set testsigning on).
)
pause
goto MENU

:STOP
:: Stop driver service
echo.
echo [INFO] Stopping driver service...
sc stop %SERVICE_NAME% >nul 2>&1
if %errorLevel% equ 0 (
    echo [SUCCESS] Driver service stopped.
) else (
    echo [ERROR] Failed to stop driver service.
)
pause
goto MENU

:EXIT
echo.
echo Exiting...
exit /b 0
