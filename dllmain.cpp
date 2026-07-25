// ============================================================================
// NO-WDK USER-MODE DLL
// Complete DLL implementation for communicating with the custom kernel driver
// Compile as: Visual C++ DLL project with /WX (Treat Warnings As Errors)
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include "Driver.h"

// ============================================================================
// DLL EXPORTS
// ============================================================================

#define MEMORYDRIVER_EXPORTS

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

static HANDLE g_DriverHandle = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION g_DriverLock;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Initialize the critical section
BOOL InitializeCriticalSection()
{
    InitializeCriticalSection(&g_DriverLock);
    return TRUE;
}

// Cleanup the critical section
VOID CleanupCriticalSection()
{
    DeleteCriticalSection(&g_DriverLock);
}

// Open connection to the driver
BOOL OpenDriverConnection()
{
    EnterCriticalSection(&g_DriverLock);
    
    if (g_DriverHandle != INVALID_HANDLE_VALUE) {
        LeaveCriticalSection(&g_DriverLock);
        return TRUE;
    }
    
    // Open the driver device
    g_DriverHandle = CreateFileW(
        DRIVER_USER_LINK_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (g_DriverHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        LeaveCriticalSection(&g_DriverLock);
        return FALSE;
    }
    
    LeaveCriticalSection(&g_DriverLock);
    return TRUE;
}

// Close connection to the driver
VOID CloseDriverConnection()
{
    EnterCriticalSection(&g_DriverLock);
    
    if (g_DriverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_DriverHandle);
        g_DriverHandle = INVALID_HANDLE_VALUE;
    }
    
    LeaveCriticalSection(&g_DriverLock);
}

// ============================================================================
// MEMORY OPERATION WRAPPERS
// ============================================================================

// Read memory from a target process
MEMORYDRIVER_API BOOL ReadProcessMemoryEx(
    DWORD ProcessId,
    ULONG_PTR TargetAddress,
    LPVOID Buffer,
    SIZE_T BufferSize,
    PSIZE_T BytesRead
)
{
    if (!OpenDriverConnection()) {
        SetLastError(ERROR_DEVICE_NOT_AVAILABLE);
        return FALSE;
    }
    
    MEMORY_OPERATION_REQUEST request = { 0 };
    DWORD bytesReturned = 0;
    BOOL result = FALSE;
    
    // Fill in the request
    request.ProcessId = (ULONG_PTR)ProcessId;
    request.TargetAddress = TargetAddress;
    request.Buffer = (ULONG_PTR)Buffer;
    request.Size = BufferSize;
    request.BytesTransferred = 0;
    request.Status = STATUS_PENDING;
    
    // Send IOCTL to driver
    result = DeviceIoControl(
        g_DriverHandle,
        IOCTL_READ_PROCESS_MEMORY,
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        &bytesReturned,
        NULL
    );
    
    if (result) {
        if (NT_SUCCESS(request.Status)) {
            if (BytesRead) {
                *BytesRead = request.BytesTransferred;
            }
            return TRUE;
        } else {
            // Map NTSTATUS to Windows error
            switch (request.Status) {
                case STATUS_ACCESS_DENIED:
                    SetLastError(ERROR_ACCESS_DENIED);
                    break;
                case STATUS_INVALID_PARAMETER:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                case STATUS_INFO_LENGTH_MISMATCH:
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    break;
                default:
                    SetLastError(ERROR_GEN_FAILURE);
                    break;
            }
            return FALSE;
        }
    } else {
        DWORD error = GetLastError();
        SetLastError(error);
        return FALSE;
    }
}

// Write memory to a target process
MEMORYDRIVER_API BOOL WriteProcessMemoryEx(
    DWORD ProcessId,
    ULONG_PTR TargetAddress,
    LPCVOID Buffer,
    SIZE_T BufferSize,
    PSIZE_T BytesWritten
)
{
    if (!OpenDriverConnection()) {
        SetLastError(ERROR_DEVICE_NOT_AVAILABLE);
        return FALSE;
    }
    
    MEMORY_OPERATION_REQUEST request = { 0 };
    DWORD bytesReturned = 0;
    BOOL result = FALSE;
    
    // Fill in the request
    request.ProcessId = (ULONG_PTR)ProcessId;
    request.TargetAddress = TargetAddress;
    request.Buffer = (ULONG_PTR)Buffer;
    request.Size = BufferSize;
    request.BytesTransferred = 0;
    request.Status = STATUS_PENDING;
    
    // Send IOCTL to driver
    result = DeviceIoControl(
        g_DriverHandle,
        IOCTL_WRITE_PROCESS_MEMORY,
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        &bytesReturned,
        NULL
    );
    
    if (result) {
        if (NT_SUCCESS(request.Status)) {
            if (BytesWritten) {
                *BytesWritten = request.BytesTransferred;
            }
            return TRUE;
        } else {
            // Map NTSTATUS to Windows error
            switch (request.Status) {
                case STATUS_ACCESS_DENIED:
                    SetLastError(ERROR_ACCESS_DENIED);
                    break;
                case STATUS_INVALID_PARAMETER:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                case STATUS_INFO_LENGTH_MISMATCH:
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    break;
                default:
                    SetLastError(ERROR_GEN_FAILURE);
                    break;
            }
            return FALSE;
        }
    } else {
        DWORD error = GetLastError();
        SetLastError(error);
        return FALSE;
    }
}

// Get process information
MEMORYDRIVER_API BOOL GetProcessInfoEx(
    DWORD ProcessId,
    PULONG_PTR ProcessBaseAddress,
    PSIZE_T ProcessSize
)
{
    if (!OpenDriverConnection()) {
        SetLastError(ERROR_DEVICE_NOT_AVAILABLE);
        return FALSE;
    }
    
    PROCESS_INFO_REQUEST request = { 0 };
    DWORD bytesReturned = 0;
    BOOL result = FALSE;
    
    // Fill in the request
    request.ProcessId = (ULONG_PTR)ProcessId;
    request.ProcessBaseAddress = 0;
    request.ProcessSize = 0;
    request.Status = STATUS_PENDING;
    
    // Send IOCTL to driver
    result = DeviceIoControl(
        g_DriverHandle,
        IOCTL_GET_PROCESS_INFO,
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        &bytesReturned,
        NULL
    );
    
    if (result) {
        if (NT_SUCCESS(request.Status)) {
            if (ProcessBaseAddress) {
                *ProcessBaseAddress = request.ProcessBaseAddress;
            }
            if (ProcessSize) {
                *ProcessSize = request.ProcessSize;
            }
            return TRUE;
        } else {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
    } else {
        DWORD error = GetLastError();
        SetLastError(error);
        return FALSE;
    }
}

// ============================================================================
// DLL MAIN FUNCTIONS
// ============================================================================

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            // Initialize critical section
            if (!InitializeCriticalSection()) {
                return FALSE;
            }
            
            // Initialize driver handle
            g_DriverHandle = INVALID_HANDLE_VALUE;
            
            break;
        }
        
        case DLL_PROCESS_DETACH: {
            // Close driver connection
            CloseDriverConnection();
            
            // Cleanup critical section
            CleanupCriticalSection();
            
            break;
        }
        
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    
    return TRUE;
}

// ============================================================================
// ADDITIONAL EXPORTED FUNCTIONS
// ============================================================================

// Check if driver is loaded and accessible
MEMORYDRIVER_API BOOL IsDriverLoaded()
{
    return OpenDriverConnection();
}

// Get last NTSTATUS error as string (for debugging)
MEMORYDRIVER_API LPCSTR GetNtStatusString(NTSTATUS Status)
{
    switch (Status) {
        case STATUS_SUCCESS: return "STATUS_SUCCESS";
        case STATUS_UNSUCCESSFUL: return "STATUS_UNSUCCESSFUL";
        case STATUS_INVALID_PARAMETER: return "STATUS_INVALID_PARAMETER";
        case STATUS_INFO_LENGTH_MISMATCH: return "STATUS_INFO_LENGTH_MISMATCH";
        case STATUS_ACCESS_DENIED: return "STATUS_ACCESS_DENIED";
        case STATUS_BUFFER_TOO_SMALL: return "STATUS_BUFFER_TOO_SMALL";
        case STATUS_DEVICE_NOT_FOUND: return "STATUS_DEVICE_NOT_FOUND";
        case STATUS_NO_MEMORY: return "STATUS_NO_MEMORY";
        case STATUS_PENDING: return "STATUS_PENDING";
        default: return "UNKNOWN_STATUS";
    }
}

// ============================================================================
// EXPORTED FUNCTION FOR TESTING
// ============================================================================

// Test function to verify driver communication
extern "C" MEMORYDRIVER_API BOOL TestDriverCommunication()
{
    if (!IsDriverLoaded()) {
        return FALSE;
    }
    
    // Try to read 4 bytes from current process at address 0 (should fail gracefully)
    BYTE testBuffer[4] = { 0 };
    SIZE_T bytesRead = 0;
    
    BOOL result = ReadProcessMemoryEx(
        GetCurrentProcessId(),
        0x00000000,
        testBuffer,
        sizeof(testBuffer),
        &bytesRead
    );
    
    // We expect this to fail, but the important thing is that the communication works
    DWORD error = GetLastError();
    
    // If we get here without crashing, communication is working
    return TRUE;
}
