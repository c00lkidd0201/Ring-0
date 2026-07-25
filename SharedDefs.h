#pragma once

// ============================================================================
// SHARED DEFINITIONS - Common for both kernel and user mode
// ============================================================================

// -----------------------------------------------------------------------------
// Basic Types - Define only if not already defined
// -----------------------------------------------------------------------------

#ifndef _SHARED_DEFS_H_
#define _SHARED_DEFS_H_

// Prevent multiple inclusions
#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Platform Detection
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE
    // Kernel mode
#else
    // User mode
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winioctl.h>
#endif

// -----------------------------------------------------------------------------
// Common Types
// -----------------------------------------------------------------------------

// If not in kernel mode and not defined, define basic types
#ifndef _WINDOWS_
#ifndef _INC_WINDOWS

// Basic integer types
typedef signed char         INT8;
typedef unsigned char       UINT8;
typedef signed short        INT16;
typedef unsigned short      UINT16;
typedef signed int          INT32;
typedef unsigned int        UINT32;
typedef signed long long    INT64;
typedef unsigned long long  UINT64;

// Windows-compatible types
typedef UINT8               BYTE;
typedef UINT16              WORD;
typedef UINT32              DWORD;
typedef INT32               LONG;
typedef UINT32              ULONG;

// Pointer types
typedef void*               PVOID;
typedef const void*         PCVOID;

// Handle types
typedef PVOID               HANDLE;

// Boolean
typedef UINT8               BOOLEAN;
#ifndef TRUE
#define TRUE                    1
#endif
#ifndef FALSE
#define FALSE                   0
#endif
#ifndef NULL
#define NULL                    ((PVOID)0)
#endif

// Size types
#ifdef _WIN64
    typedef UINT64              SIZE_T;
    typedef INT64               SSIZE_T;
    typedef UINT64              ULONG_PTR;
    typedef INT64               LONG_PTR;
#else
    typedef UINT32              SIZE_T;
    typedef INT32               SSIZE_T;
    typedef UINT32              ULONG_PTR;
    typedef INT32               LONG_PTR;
#endif

// NTSTATUS
#ifndef _NTSTATUS_DEFINED_
#define _NTSTATUS_DEFINED_
typedef LONG                NTSTATUS;
#endif

#endif // !_WINDOWS_ && !_INC_WINDOWS
#endif // !_KERNEL_MODE

// -----------------------------------------------------------------------------
// NT Status Codes
// -----------------------------------------------------------------------------

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
#endif
#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL          ((NTSTATUS)0xC0000001L)
#endif
#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER     ((NTSTATUS)0xC000000DL)
#endif
#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
#endif
#ifndef STATUS_ACCESS_DENIED
#define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
#endif
#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL      ((NTSTATUS)0xC0000023L)
#endif
#ifndef STATUS_DEVICE_NOT_FOUND
#define STATUS_DEVICE_NOT_FOUND      ((NTSTATUS)0xC000000EL)
#endif
#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY             ((NTSTATUS)0xC0000017L)
#endif
#ifndef STATUS_PENDING
#define STATUS_PENDING               ((NTSTATUS)0x00000103L)
#endif
#ifndef STATUS_NOT_IMPLEMENTED
#define STATUS_NOT_IMPLEMENTED       ((NTSTATUS)0xC0000001L)
#endif

// -----------------------------------------------------------------------------
// Error Handling Macros
// -----------------------------------------------------------------------------

#define NT_SUCCESS(Status)           (((NTSTATUS)(Status)) >= 0)
#define NT_FAILURE(Status)           (((NTSTATUS)(Status)) < 0)

// -----------------------------------------------------------------------------
// IOCTL Definitions
// -----------------------------------------------------------------------------

// Manual CTL_CODE macro definition
#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// Device Type - Custom unique value (0x8000-0xFFFF range)
#define FILE_DEVICE_CUSTOM_DRIVER      0x8000

// Access Rights
#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS                0x0000
#endif
#ifndef FILE_READ_ACCESS
#define FILE_READ_ACCESS               0x0001
#endif
#ifndef FILE_WRITE_ACCESS
#define FILE_WRITE_ACCESS              0x0002
#endif
#ifndef FILE_READ_WRITE_ACCESS
#define FILE_READ_WRITE_ACCESS         0x0003
#endif

// Method Codes
#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED                0
#endif
#ifndef METHOD_IN_DIRECT
#define METHOD_IN_DIRECT               1
#endif
#ifndef METHOD_OUT_DIRECT
#define METHOD_OUT_DIRECT              2
#endif
#ifndef METHOD_NEITHER
#define METHOD_NEITHER                 3
#endif

// Custom IOCTL Codes
#define IOCTL_READ_PROCESS_MEMORY      CTL_CODE(FILE_DEVICE_CUSTOM_DRIVER, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRITE_PROCESS_MEMORY     CTL_CODE(FILE_DEVICE_CUSTOM_DRIVER, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GET_PROCESS_INFO         CTL_CODE(FILE_DEVICE_CUSTOM_DRIVER, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

// -----------------------------------------------------------------------------
// Memory Operation Structures
// -----------------------------------------------------------------------------

#pragma pack(push, 1)

// Structure for read/write process memory requests
typedef struct _MEMORY_OPERATION_REQUEST {
    ULONG_PTR ProcessId;           // Target process ID
    ULONG_PTR TargetAddress;       // Address to read from/write to
    ULONG_PTR Buffer;             // Buffer pointer (user-mode address)
    SIZE_T    Size;               // Size of data to read/write
    SIZE_T    BytesTransferred;    // Actual bytes read/written (output)
    NTSTATUS  Status;             // Operation status (output)
} MEMORY_OPERATION_REQUEST, *PMEMORY_OPERATION_REQUEST;

// Structure for process information
typedef struct _PROCESS_INFO_REQUEST {
    ULONG_PTR ProcessId;           // Process ID to query
    ULONG_PTR ProcessBaseAddress;  // Base address of process (output)
    SIZE_T    ProcessSize;        // Size of process image (output)
    NTSTATUS  Status;             // Operation status (output)
} PROCESS_INFO_REQUEST, *PPROCESS_INFO_REQUEST;

#pragma pack(pop)

// -----------------------------------------------------------------------------
// Symbolic Link and Device Names
// -----------------------------------------------------------------------------

#define DRIVER_DEVICE_NAME            L"\\Device\\CustomMemoryDriver"
#define DRIVER_SYMBOLIC_LINK_NAME     L"\\DosDevices\\CustomMemoryDriver"
#define DRIVER_USER_LINK_NAME         L"\\\\.\\CustomMemoryDriver"

// -----------------------------------------------------------------------------
// DLL Export Definitions
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE
    // Kernel-mode: no DLL exports
#else
    #ifdef MEMORYDRIVER_EXPORTS
        #define MEMORYDRIVER_API __declspec(dllexport)
    #else
        #define MEMORYDRIVER_API __declspec(dllimport)
    #endif
#endif

// -----------------------------------------------------------------------------
// Alignment Macros
// -----------------------------------------------------------------------------

#define ALIGN_DOWN_POINTER(p, align) ((PVOID)((ULONG_PTR)(p) & ~((ULONG_PTR)(align) - 1)))
#define ALIGN_UP_POINTER(p, align)   ((PVOID)(((ULONG_PTR)(p) + (ULONG_PTR)(align) - 1) & ~((ULONG_PTR)(align) - 1)))

#ifdef __cplusplus
}
#endif

#endif // _SHARED_DEFS_H_
