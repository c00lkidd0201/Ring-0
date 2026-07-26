@echo off
chcp 65001 >nul
echo ========================================
echo Ring-0 Driver Test
 echo Windows 11 x64
 echo ========================================
echo.

set DLL_PATH=%~dp0Release\Executor\RobloxExecutor.dll

echo Checking DLL...
if not exist "%DLL_PATH%" (
    echo Error: RobloxExecutor.dll not found at: %DLL_PATH%
    echo Please build the Executor DLL first
    pause
    exit /b 1
)

echo Creating test application...
set TEST_APP=%~dp0test_driver.exe

if exist "%TEST_APP%" (
    del "%TEST_APP%" >nul 2>&1
)

echo @echo off > "%TEMP%\test_driver_temp.bat"
echo chcp 65001 ^>nul >> "%TEMP%\test_driver_temp.bat"
echo setlocal >> "%TEMP%\test_driver_temp.bat"
echo set DLL_PATH=%DLL_PATH% >> "%TEMP%\test_driver_temp.bat"
echo echo Testing driver connection... >> "%TEMP%\test_driver_temp.bat"
echo echo. >> "%TEMP%\test_driver_temp.bat"

call :CreateTestApp

if not exist "%TEST_APP%" (
    echo Error: Failed to create test application
    pause
    exit /b 1
)

echo Running test...
echo.
"%TEST_APP%"

echo.
echo Test completed!
echo Check DebugView for detailed logs
echo.
pause
goto :eof

:CreateTestApp
set C_FILE=%TEMP%\test_driver.c
set EXE_FILE=%TEST_APP%

(
echo #include ^<Windows.h^>
echo #include ^<stdio.h^>
echo.
echo // Import functions from DLL
echo typedef bool (__cdecl *PFN_IS_DRIVER_LOADED)();
echo typedef bool (__cdecl *PFN_TEST_DRIVER_CONNECTION)();
echo.
echo int main() {
echo     HMODULE hDll = LoadLibraryA("%DLL_PATH%");
echo     if (!hDll) {
echo         printf("Failed to load DLL\n");
echo         return 1;
echo     }
echo.
echo     auto IsDriverLoaded = (PFN_IS_DRIVER_LOADED)GetProcAddress(hDll, "IsDriverLoaded");
echo     auto TestDriverConnection = (PFN_TEST_DRIVER_CONNECTION)GetProcAddress(hDll, "TestDriverConnection");
echo.
echo     if (!IsDriverLoaded || !TestDriverConnection) {
echo         printf("Failed to get function pointers\n");
echo         FreeLibrary(hDll);
echo         return 1;
echo     }
echo.
echo     printf("Testing driver...\n\n");
echo.
echo     if (IsDriverLoaded()) {
echo         printf("[+] Driver is loaded\n");
echo         if (TestDriverConnection()) {
echo             printf("[+] Driver connection test: SUCCESS\n");
echo         } else {
echo             printf("[-] Driver connection test: FAILED\n");
echo         }
echo     } else {
echo         printf("[-] Driver is not loaded\n");
echo     }
echo.
echo     FreeLibrary(hDll);
echo     printf("\nPress any key to exit...");
echo     getchar();
echo     return 0;
echo }
) > "%C_FILE%"

set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build
if exist "%VCINSTALLDIR%\vcvars64.bat" (
    call "%VCINSTALLDIR%\vcvars64.bat" >nul 2>&1
    cl /EHsc /Fe:"%EXE_FILE%" "%C_FILE%" kernel32.lib user32.lib >nul 2>&1
    if exist "%EXE_FILE%" (
        del "%C_FILE%" >nul 2>&1
        goto :eof
    )
)

:: Fallback: Use gcc if available
where gcc >nul 2>&1
if errorlevel 1 (
    echo Warning: Neither Visual Studio nor gcc found
    echo Trying to use cl.exe from PATH...
    cl /EHsc /Fe:"%EXE_FILE%" "%C_FILE%" kernel32.lib user32.lib >nul 2>&1
)

if exist "%EXE_FILE%" (
    del "%C_FILE%" >nul 2>&1
)
goto :eof
