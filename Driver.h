#pragma once

// ============================================================================
// NO-WDK DRIVER HEADER - Universal header for kernel and user mode
// ============================================================================

// -----------------------------------------------------------------------------
// Platform Detection
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE
    // Kernel mode - no Windows headers
#else
    // User mode - include Windows headers
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winioctl.h>
#endif

// -----------------------------------------------------------------------------
// Basic Types - Must be defined for kernel mode
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE

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
typedef INT64               LONG64;
typedef UINT64              ULONG64;

// Pointer types
typedef void*               PVOID;
typedef const void*         PCVOID;
typedef char*               PCHAR;
typedef const char*         PCCHAR;
typedef wchar_t*            PWCHAR;
typedef const wchar_t*     PCWCHAR;

// Handle types
typedef PVOID               HANDLE;

// Boolean
typedef UINT8               BOOLEAN;
#define TRUE                    1
#define FALSE                   0
#define NULL                    ((PVOID)0)

// Size types
typedef UINT64              SIZE_T;
typedef UINT64              ULONG_PTR;
typedef INT64               LONG_PTR;
typedef INT64               SSIZE_T;

// NTSTATUS
typedef LONG                NTSTATUS;

// Calling conventions
#define NTAPI                   __stdcall
#define NTINLINE                __inline

// Memory pool types
#define NonPagedPoolNx          0x00000020UL

#else

// User mode - ensure we have all types
typedef unsigned long long  ULONG_PTR;
typedef long long           LONG_PTR;
typedef long                NTSTATUS;

#endif // _KERNEL_MODE

// -----------------------------------------------------------------------------
// NT Status Codes (common for both modes)
// -----------------------------------------------------------------------------

#ifndef _NTSTATUS_DEFINED_
#define _NTSTATUS_DEFINED_

define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL          ((NTSTATUS)0xC0000001L)
#define STATUS_INVALID_PARAMETER     ((NTSTATUS)0xC000000DL)
#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
#define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
#define STATUS_BUFFER_TOO_SMALL      ((NTSTATUS)0xC0000023L)
#define STATUS_DEVICE_NOT_FOUND      ((NTSTATUS)0xC000000EL)
#define STATUS_NO_MEMORY             ((NTSTATUS)0xC0000017L)
#define STATUS_PENDING               ((NTSTATUS)0x00000103L)
#define STATUS_NOT_IMPLEMENTED       ((NTSTATUS)0xC0000001L)
#endif

// -----------------------------------------------------------------------------
// String Structures (for kernel mode)
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE

// UNICODE_STRING structure
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

// ANSI_STRING structure
typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} ANSI_STRING, *PANSI_STRING;

// LARGE_INTEGER structure
typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    struct {
        ULONG LowPart;
        LONG HighPart;
    } u;
    LONG64 QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#else

// User mode - these are already defined in windows.h
// But we need to ensure compatibility

#endif // _KERNEL_MODE

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

// Access Rights (avoid redefinition in user mode)
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

// Structure for read/write process memory requests
#pragma pack(push, 1)
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
// Error Handling Macros
// -----------------------------------------------------------------------------

#define NT_SUCCESS(Status)           (((NTSTATUS)(Status)) >= 0)
#define NT_FAILURE(Status)           (((NTSTATUS)(Status)) < 0)

// -----------------------------------------------------------------------------
// Alignment Macros
// -----------------------------------------------------------------------------

#define ALIGN_DOWN_POINTER(p, align) ((PVOID)((ULONG_PTR)(p) & ~((ULONG_PTR)(align) - 1)))
#define ALIGN_UP_POINTER(p, align)   ((PVOID)(((ULONG_PTR)(p) + (ULONG_PTR)(align) - 1) & ~((ULONG_PTR)(align) - 1)))

// -----------------------------------------------------------------------------
// Prevent macro redefinition warnings in user mode
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE
// Kernel mode definitions go here
#else
// User mode - prevent redefinition of Windows types
#undef MEMORYDRIVER_EXPORTS
#endif
