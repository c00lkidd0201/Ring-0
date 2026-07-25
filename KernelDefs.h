#pragma once

// ============================================================================
// KERNEL DEFINITIONS - Only for kernel mode
// ============================================================================

#ifndef _KERNEL_DEFS_H_
#define _KERNEL_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Basic Types for Kernel Mode
// ============================================================================

typedef signed char         INT8;
typedef unsigned char       UINT8;
typedef signed short        INT16;
typedef unsigned short      UINT16;
typedef signed int          INT32;
typedef unsigned int        UINT32;
typedef signed long long    INT64;
typedef unsigned long long  UINT64;

typedef UINT8               BYTE;
typedef UINT16              WORD;
typedef UINT32              DWORD;
typedef INT32               LONG;
typedef UINT32              ULONG;
typedef INT64               LONG64;
typedef UINT64              ULONG64;

typedef void*               PVOID;
typedef const void*         PCVOID;
typedef char*               PCHAR;
typedef const char*         PCCHAR;
typedef wchar_t*            PWCHAR;
typedef const wchar_t*     PCWCHAR;

typedef PVOID               HANDLE;

typedef UINT8               BOOLEAN;
#define TRUE                    1
#define FALSE                   0
#define NULL                    ((PVOID)0)

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

typedef LONG                NTSTATUS;

#define NTAPI                   __stdcall
#define NTINLINE                __inline

// ============================================================================
// String Structures
// ============================================================================

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    LONG64 QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

// ============================================================================
// Object Structures
// ============================================================================

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT DEVICE_OBJECT, *PDEVICE_OBJECT;
typedef struct _IRP IRP, *PIRP;
typedef struct _IO_STACK_LOCATION IO_STACK_LOCATION, *PIO_STACK_LOCATION;
typedef struct _EPROCESS EPROCESS, *PEPROCESS;
typedef struct _KPROCESS KPROCESS, *PKPROCESS;

// ============================================================================
// DEVICE_OBJECT Structure
// ============================================================================

typedef struct _DEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT NextDevice;
    PDEVICE_OBJECT AttachedDevice;
    PIRP CurrentIrp;
    PVOID Timer;
    ULONG Flags;
    ULONG Characteristics;
    PVOID DeviceExtension;
    ULONG DeviceType;
    CCHAR StackSize;
    union {
        PDEVICE_OBJECT Next;
        PVOID Vpb;
    };
    PVOID DeviceObjectExtension;
    PVOID Reserved;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

// ============================================================================
// DRIVER_OBJECT Structure
// ============================================================================

typedef VOID (NTAPI *PDRIVER_UNLOAD)(PDRIVER_OBJECT DriverObject);
typedef NTSTATUS (NTAPI *PDRIVER_DISPATCH)(PDEVICE_OBJECT DeviceObject, PIRP Irp);

typedef struct _DRIVER_OBJECT {
    CSHORT Type;
    USHORT Size;
    PDEVICE_OBJECT DeviceObject;
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
    PVOID DriverSection;
    PVOID DriverExtension;
    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_DISPATCH MajorFunction[28]; // IRP_MJ_MAXIMUM_FUNCTION + 1
} DRIVER_OBJECT, *PDRIVER_OBJECT;

// ============================================================================
// IRP Structure
// ============================================================================

typedef struct _IRP {
    CSHORT Type;
    USHORT Size;
    PVOID MdlAddress;
    ULONG Flags;
    union {
        PIRP AssociatedIrp;
        PVOID Thread;
    };
    PIO_STACK_LOCATION StackLocation;
    PVOID UserBuffer;
    union {
        struct {
            PVOID UserApcRoutine;
            PVOID UserApcContext;
        };
        PVOID UserEvent;
    };
    PVOID UserIosb;
    PVOID UserIosbValue;
    ULONG Overlay;
    PVOID CancelRoutine;
    PVOID UserBuffer2;
    PVOID Reserved;
    PVOID CancelIrql;
    PVOID Apc;
    PVOID Notification;
} IRP, *PIRP;

// ============================================================================
// IO_STACK_LOCATION Structure
// ============================================================================

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;
    union {
        struct {
            PVOID DeviceObject;
            PVOID FileObject;
            PVOID CompletionRoutine;
            PVOID Context;
        };
        struct {
            PVOID DeviceObject;
            ULONG_PTR Parameters;
        };
    };
    PVOID DeviceObject;
    PVOID FileObject;
    union {
        struct {
            PVOID Read;
            PVOID Write;
        };
        struct {
            ULONG_PTR Length;
            ULONG_PTR Key;
            LARGE_INTEGER ByteOffset;
        };
        struct {
            ULONG_PTR Length;
            PVOID Buffer;
        };
    } Parameters;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

// ============================================================================
// IRP Major Function Codes
// ============================================================================

typedef enum _IRP_MJ {
    IRP_MJ_CREATE = 0x00,
    IRP_MJ_CREATE_NAMED_PIPE = 0x01,
    IRP_MJ_CLOSE = 0x02,
    IRP_MJ_READ = 0x03,
    IRP_MJ_WRITE = 0x04,
    IRP_MJ_QUERY_INFORMATION = 0x05,
    IRP_MJ_SET_INFORMATION = 0x06,
    IRP_MJ_QUERY_EA = 0x07,
    IRP_MJ_SET_EA = 0x08,
    IRP_MJ_FLUSH_BUFFERS = 0x09,
    IRP_MJ_QUERY_VOLUME_INFORMATION = 0x0A,
    IRP_MJ_SET_VOLUME_INFORMATION = 0x0B,
    IRP_MJ_DIRECTORY_CONTROL = 0x0C,
    IRP_MJ_FILE_SYSTEM_CONTROL = 0x0D,
    IRP_MJ_DEVICE_CONTROL = 0x0E,
    IRP_MJ_INTERNAL_DEVICE_CONTROL = 0x0F,
    IRP_MJ_SHUTDOWN = 0x10,
    IRP_MJ_LOCK_CONTROL = 0x11,
    IRP_MJ_CLEANUP = 0x12,
    IRP_MJ_CREATE_MAILSLOT = 0x13,
    IRP_MJ_QUERY_SECURITY = 0x14,
    IRP_MJ_SET_SECURITY = 0x15,
    IRP_MJ_POWER = 0x16,
    IRP_MJ_SYSTEM_CONTROL = 0x17,
    IRP_MJ_DEVICE_CHANGE = 0x18,
    IRP_MJ_QUERY_QUOTA = 0x19,
    IRP_MJ_SET_QUOTA = 0x1A,
    IRP_MJ_PNP = 0x1B,
    IRP_MJ_MAXIMUM_FUNCTION = 0x1C
} IRP_MJ;

// ============================================================================
// Kernel API Declarations
// ============================================================================

// Memory Management
__declspec(dllimport) PVOID NTAPI ExAllocatePoolWithTag(ULONG PoolType, SIZE_T NumberOfBytes, ULONG Tag);
__declspec(dllimport) VOID NTAPI ExFreePoolWithTag(PVOID P, ULONG Tag);

// String Functions
__declspec(dllimport) VOID NTAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);
__declspec(dllimport) NTSTATUS NTAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING DestinationString, PANSI_STRING SourceString, BOOLEAN AllocateDestination);
__declspec(dllimport) VOID NTAPI RtlFreeUnicodeString(PUNICODE_STRING UnicodeString);
__declspec(dllimport) VOID NTAPI RtlCopyMemory(PVOID Destination, PCVOID Source, SIZE_T Length);

// Device and Driver Management
__declspec(dllimport) NTSTATUS NTAPI IoCreateDevice(
    PDRIVER_OBJECT DriverObject,
    ULONG DeviceExtensionSize,
    PUNICODE_STRING DeviceName,
    ULONG DeviceType,
    ULONG DeviceCharacteristics,
    BOOLEAN Exclusive,
    PDEVICE_OBJECT *DeviceObject
);

__declspec(dllimport) NTSTATUS NTAPI IoCreateSymbolicLink(
    PUNICODE_STRING SymbolicLinkName,
    PUNICODE_STRING DeviceName
);

__declspec(dllimport) VOID NTAPI IoDeleteDevice(PDEVICE_OBJECT DeviceObject);
__declspec(dllimport) NTSTATUS NTAPI IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName);

// IRP Handling
__declspec(dllimport) VOID NTAPI IoCompleteRequest(PIRP Irp, CCHAR PriorityBoost);
__declspec(dllimport) PIO_STACK_LOCATION NTAPI IoGetCurrentIrpStackLocation(PIRP Irp);

// Memory Copy for cross-process access
__declspec(dllimport) NTSTATUS NTAPI MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    ULONG PreviousMode,
    PSIZE_T ReturnSize
);

// Process Management
__declspec(dllimport) NTSTATUS NTAPI PsLookupProcessByProcessId(
    HANDLE ProcessId,
    PEPROCESS *Process
);

__declspec(dllimport) VOID NTAPI ObDereferenceObject(PVOID Object);

// ============================================================================
// Constants
// ============================================================================

#define FILE_DEVICE_SECURE_OPEN       0x00000001
#define IO_NO_INCREMENT               0

// Memory Tag for pool allocation
#define MEMORY_DRIVER_TAG             'DvMm'

// ============================================================================
// Helper Macros
// ============================================================================

#define UNREFERENCED_PARAMETER(P)     (void)(P)

#ifdef __cplusplus
}
#endif

#endif // _KERNEL_DEFS_H_
