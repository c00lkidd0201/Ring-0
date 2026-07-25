#pragma once

// ============================================================================
// NO-WDK DRIVER HEADER
// Manual definitions for IOCTL communication between user-mode DLL and kernel driver
// ============================================================================

// -----------------------------------------------------------------------------
// Basic Windows Types (NO <windows.h> in kernel mode)
// -----------------------------------------------------------------------------

#ifdef _KERNEL_MODE
    typedef signed char         INT8;
    typedef unsigned char       UINT8;
    typedef signed short        INT16;
    typedef unsigned short      UINT16;
    typedef signed int          INT32;
    typedef unsigned int        UINT32;
    typedef signed long long    INT64;
    typedef unsigned long long  UINT64;
    
    typedef UINT8               BOOLEAN;
    typedef UINT32              ULONG;
    typedef UINT64              ULONG_PTR;
    typedef void*               PVOID;
    typedef const void*         PCVOID;
    typedef UINT32              NTSTATUS;
    typedef UINT32              SIZE_T;
    
    #define NULL                ((PVOID)0)
    #define TRUE                1
    #define FALSE               0
    
    // NT Status Codes (partial, manual definitions)
    #define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
    #define STATUS_UNSUCCESSFUL          ((NTSTATUS)0xC0000001L)
    #define STATUS_INVALID_PARAMETER     ((NTSTATUS)0xC000000DL)
    #define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
    #define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
    #define STATUS_BUFFER_TOO_SMALL      ((NTSTATUS)0xC0000023L)
    #define STATUS_DEVICE_NOT_FOUND      ((NTSTATUS)0xC000000EL)
    #define STATUS_NO_MEMORY             ((NTSTATUS)0xC0000017L)
    #define STATUS_PENDING               ((NTSTATUS)0x00000103L)
    
    // Pool Types for memory allocation
    #define NonPagedPoolNx              0x00000020UL
    
    // UNICODE_STRING structure
    typedef struct _UNICODE_STRING {
        UINT16 Length;
        UINT16 MaximumLength;
        PWCHAR Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;
    
    typedef const UNICODE_STRING* PCUNICODE_STRING;
    
    // ANSI_STRING structure
    typedef struct _ANSI_STRING {
        UINT16 Length;
        UINT16 MaximumLength;
        PCHAR Buffer;
    } ANSI_STRING, *PANSI_STRING;
    
    // OBJECT_ATTRIBUTES structure
    typedef struct _OBJECT_ATTRIBUTES {
        ULONG Length;
        HANDLE RootDirectory;
        PUNICODE_STRING ObjectName;
        ULONG Attributes;
        PVOID SecurityDescriptor;
        PVOID SecurityQualityOfService;
    } OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
    
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
    
    // FILE_TIME structure
    typedef struct _FILETIME {
        UINT32 dwLowDateTime;
        UINT32 dwHighDateTime;
    } FILETIME, *PFILETIME;

#else
    // User-mode: include standard Windows headers
    #include <windows.h>
    #include <winioctl.h>
    
    typedef DWORD NTSTATUS;
    
    // NT Status Codes (user-mode compatible)
    #define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
    #define STATUS_UNSUCCESSFUL          ((NTSTATUS)0xC0000001L)
    #define STATUS_INVALID_PARAMETER     ((NTSTATUS)0xC000000DL)
    #define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
    #define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
    #define STATUS_BUFFER_TOO_SMALL      ((NTSTATUS)0xC0000023L)
    #define STATUS_DEVICE_NOT_FOUND      ((NTSTATUS)0xC000000EL)
    #define STATUS_NO_MEMORY             ((NTSTATUS)0xC0000017L)
    #define STATUS_PENDING               ((NTSTATUS)0x00000103L)
    
    // Define ULONG_PTR for user-mode
    #ifdef _WIN64
        typedef unsigned long long ULONG_PTR;
    #else
        typedef unsigned long ULONG_PTR;
    #endif
#endif

// ============================================================================
// IOCTL DEFINITIONS
// ============================================================================

// Manual CTL_CODE macro definition (NO <winioctl.h> in kernel)
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

// Device Type - Custom unique value (0x8000-0xFFFF range)
#define FILE_DEVICE_CUSTOM_DRIVER      0x8000

// Access Rights
#define FILE_ANY_ACCESS                0x0000
#define FILE_READ_ACCESS               0x0001
#define FILE_WRITE_ACCESS              0x0002
#define FILE_READ_WRITE_ACCESS         0x0003

// Method Codes
#define METHOD_BUFFERED                0
#define METHOD_IN_DIRECT               1
#define METHOD_OUT_DIRECT              2
#define METHOD_NEITHER                 3

// Custom IOCTL Codes
#define IOCTL_READ_PROCESS_MEMORY      CTL_CODE(FILE_DEVICE_CUSTOM_DRIVER, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRITE_PROCESS_MEMORY     CTL_CODE(FILE_DEVICE_CUSTOM_DRIVER, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GET_PROCESS_INFO         CTL_CODE(FILE_DEVICE_CUSTOM_DRIVER, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

// ============================================================================
// MEMORY OPERATION STRUCTURES
// ============================================================================

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

// ============================================================================
// SYMBOLIC LINK AND DEVICE NAMES
// ============================================================================

#define DRIVER_DEVICE_NAME            L"\\Device\\CustomMemoryDriver"
#define DRIVER_SYMBOLIC_LINK_NAME     L"\\DosDevices\\CustomMemoryDriver"
#define DRIVER_USER_LINK_NAME         L"\\\\.\\CustomMemoryDriver"

// ============================================================================
// DLL EXPORT DEFINITIONS
// ============================================================================

#ifdef _KERNEL_MODE
    // Kernel-mode: no DLL exports
#else
    #ifdef MEMORYDRIVER_EXPORTS
        #define MEMORYDRIVER_API __declspec(dllexport)
    #else
        #define MEMORYDRIVER_API __declspec(dllimport)
    #endif
#endif

// ============================================================================
// ERROR HANDLING MACROS
// ============================================================================

#define NT_SUCCESS(Status)           (((NTSTATUS)(Status)) >= 0)
#define NT_FAILURE(Status)           (((NTSTATUS)(Status)) < 0)

// ============================================================================
// ALIGNMENT MACROS
// ============================================================================

#define ALIGN_DOWN_POINTER(p, align) ((PVOID)((ULONG_PTR)(p) & ~((ULONG_PTR)(align) - 1)))
#define ALIGN_UP_POINTER(p, align)   ((PVOID)(((ULONG_PTR)(p) + (ULONG_PTR)(align) - 1) & ~((ULONG_PTR)(align) - 1)))
