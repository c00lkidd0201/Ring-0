#pragma once

// ============================================
// Ring-0 Driver Header
// Windows 11 x64 | Release | No WDK
// ============================================

#include <ntddk.h>
#include <ntifs.h>

// ============================================
// Debug Logging (DebugView)
// ============================================

#define DRIVER_PREFIX "[Ring0] "

#ifdef _DEBUG
#define DEBUG_LOG(fmt, ...) DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, DRIVER_PREFIX fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif

#define ERROR_LOG(fmt, ...) DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, DRIVER_PREFIX "ERROR: " fmt "\n", ##__VA_ARGS__)

// ============================================
// Device & Symbolic Link Names
// ============================================

#define DEVICE_NAME L"\\Device\\Ring0Driver"
#define SYMBOLIC_LINK L"\\DosDevices\\Ring0Driver"

// ============================================
// IO Control Codes
// ============================================

#define IOCTL_BASE 0x8000

// Read Memory
#define IOCTL_READ_MEMORY CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x1,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

// Write Memory  
#define IOCTL_WRITE_MEMORY CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x2,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

// Get Process Base Address
#define IOCTL_GET_PROCESS_BASE CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x3,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

// Get Module Base Address
#define IOCTL_GET_MODULE_BASE CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x4,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

// Test Connection
#define IOCTL_TEST_CONNECTION CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x5,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

// ============================================
// Request/Response Structures
// ============================================

typedef struct _READ_MEMORY_REQUEST {
    ULONG64 Address;
    SIZE_T Size;
} READ_MEMORY_REQUEST, *PREAD_MEMORY_REQUEST;

typedef struct _WRITE_MEMORY_REQUEST {
    ULONG64 Address;
    SIZE_T Size;
    UCHAR Data[1]; // Variable size
} WRITE_MEMORY_REQUEST, *PWRITE_MEMORY_REQUEST;

typedef struct _GET_PROCESS_BASE_REQUEST {
    ULONG ProcessId;
} GET_PROCESS_BASE_REQUEST, *PGET_PROCESS_BASE_REQUEST;

typedef struct _GET_MODULE_BASE_REQUEST {
    ULONG ProcessId;
    CHAR ModuleName[256];
} GET_MODULE_BASE_REQUEST, *PGET_MODULE_BASE_REQUEST;

typedef struct _MEMORY_RESPONSE {
    NTSTATUS Status;
    SIZE_T BytesRead;
    UCHAR Data[1]; // Variable size
} MEMORY_RESPONSE, *PMEMORY_RESPONSE;

typedef struct _ADDRESS_RESPONSE {
    NTSTATUS Status;
    ULONG64 Address;
} ADDRESS_RESPONSE, *PADDRESS_RESPONSE;

// ============================================
// Function Prototypes
// ============================================

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject);
VOID DeleteDevice(PDRIVER_OBJECT DriverObject);

NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS HandleReadMemory(PREAD_MEMORY_REQUEST Request, PMEMORY_RESPONSE Response, SIZE_T ResponseSize);
NTSTATUS HandleWriteMemory(PWRITE_MEMORY_REQUEST Request);
NTSTATUS HandleGetProcessBase(PGET_PROCESS_BASE_REQUEST Request, PADDRESS_RESPONSE Response);
NTSTATUS HandleGetModuleBase(PGET_MODULE_BASE_REQUEST Request, PADDRESS_RESPONSE Response);
NTSTATUS HandleTestConnection(PADDRESS_RESPONSE Response);

// ============================================
// Memory Utilities
// ============================================

BOOLEAN IsValidAddress(PVOID Address, SIZE_T Size);
NTSTATUS SafeCopyMemory(PVOID Destination, PVOID Source, SIZE_T Size);

// ============================================
// Process Utilities
// ============================================

NTSTATUS GetProcessBaseAddress(ULONG ProcessId, PULONG64 BaseAddress);
NTSTATUS GetModuleBaseAddress(ULONG ProcessId, PCSTR ModuleName, PULONG64 BaseAddress);

// ============================================
// Global Variables
// ============================================

extern PDEVICE_OBJECT g_DeviceObject;
extern UNICODE_STRING g_DeviceName;
extern UNICODE_STRING g_SymbolicLinkName;
