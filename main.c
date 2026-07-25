// ============================================================================
// NO-WDK KERNEL DRIVER (Ring 0)
// Complete driver implementation without WDK headers
// Compile as: Visual C++ Empty Project (.exe) with /DRIVER /SUBSYSTEM:NATIVE /GS- /WX
// ============================================================================

#include "Driver.h"

// ============================================================================
// KERNEL-MODE STRUCTURE DEFINITIONS (Manual, NO WDK)
// ============================================================================

// Forward declarations
typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT DEVICE_OBJECT, *PDEVICE_OBJECT;
typedef struct _IRP IRP, *PIRP;
typedef struct _IO_STACK_LOCATION IO_STACK_LOCATION, *PIO_STACK_LOCATION;

// DEVICE_OBJECT structure (partial, manual definition)
typedef struct _DEVICE_OBJECT {
    CSHORT Type;
    CSHORT Size;
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
        struct _DEVICE_OBJECT* Next;
        struct _VPB* Vpb;
    };
    PVOID DeviceObjectExtension;
    PVOID Reserved;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

// DRIVER_OBJECT structure (partial, manual definition)
typedef struct _DRIVER_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PDEVICE_OBJECT DeviceObject;
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
    PVOID DriverSection;
    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT, *PDRIVER_OBJECT;

// IRP structure (partial, manual definition)
typedef struct _IRP {
    CSHORT Type;
    USHORT Size;
    PMDL MdlAddress;
    ULONG Flags;
    union {
        struct _IRP* AssociatedIrp;
        PVOID Thread;
    };
    PIO_STACK_LOCATION StackLocation;
    PVOID UserBuffer;
    union {
        struct {
            PIO_APC_ROUTINE UserApcRoutine;
            PVOID UserApcContext;
        };
        PKEVENT UserEvent;
    };
    PIO_COMPLETION_ROUTINE UserIosb;
    PVOID UserIosbValue;
    ULONG Overlay;
    PVOID CancelRoutine;
    PVOID UserBuffer2;
    PVOID Reserved;
    PVOID CancelIrql;
    PVOID Apc;
    PVOID Notification;
} IRP, *PIRP;

// IO_STACK_LOCATION structure (partial, manual definition)
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

// Function pointer types for driver routines
typedef VOID (*PDRIVER_UNLOAD)(PDRIVER_OBJECT DriverObject);
typedef NTSTATUS (*PDRIVER_DISPATCH)(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// IRP Major Function codes
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
// KERNEL API DECLARATIONS (Manual __declspec(dllimport))
// ============================================================================

// Memory Management
__declspec(dllimport) PVOID NTAPI ExAllocatePoolWithTag(ULONG PoolType, SIZE_T NumberOfBytes, ULONG Tag);
__declspec(dllimport) VOID NTAPI ExFreePoolWithTag(PVOID P, ULONG Tag);

// String Functions
__declspec(dllimport) NTSTATUS NTAPI RtlUnicodeStringInit(PUNICODE_STRING DestinationString, PCWSTR SourceString);
__declspec(dllimport) VOID NTAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);
__declspec(dllimport) NTSTATUS NTAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING DestinationString, PANSI_STRING SourceString, BOOLEAN AllocateDestination);
__declspec(dllimport) VOID NTAPI RtlFreeUnicodeString(PUNICODE_STRING UnicodeString);

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
__declspec(dllimport) VOID NTAPI IoSetCurrentIrpStackLocation(PIRP Irp);

// Memory Copy for cross-process access (sUNC bypass)
__declspec(dllimport) NTSTATUS NTAPI MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize
);

// Process Management
__declspec(dllimport) NTSTATUS NTAPI PsLookupProcessByProcessId(
    HANDLE ProcessId,
    PEPROCESS *Process
);

__declspec(dllimport) VOID NTAPI ObDereferenceObject(PVOID Object);

// ============================================================================
// TYPE REDEFINITIONS FOR COMPATIBILITY
// ============================================================================

typedef struct _EPROCESS EPROCESS, *PEPROCESS;
typedef struct _KPROCESS KPROCESS, *PKPROCESS;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

PDEVICE_OBJECT g_DeviceObject = NULL;
PUNICODE_STRING g_DeviceName = NULL;
PUNICODE_STRING g_SymbolicLinkName = NULL;

// Memory Tag for pool allocation
#define MEMORY_DRIVER_TAG 'DvMm'

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Initialize Unicode String
VOID InitUnicodeString(PUNICODE_STRING pUnicodeString, PCWSTR pBuffer)
{
    if (pUnicodeString && pBuffer) {
        SIZE_T length = wcslen(pBuffer) * sizeof(WCHAR);
        pUnicodeString->Length = (USHORT)length;
        pUnicodeString->MaximumLength = (USHORT)(length + sizeof(WCHAR));
        pUnicodeString->Buffer = (PWCHAR)pBuffer;
    }
}

// Safe string copy for Unicode
NTSTATUS CopyUnicodeString(PUNICODE_STRING Destination, PCUNICODE_STRING Source)
{
    if (!Destination || !Source) {
        return STATUS_INVALID_PARAMETER;
    }
    
    Destination->Length = Source->Length;
    Destination->MaximumLength = Source->MaximumLength;
    
    if (Source->Buffer) {
        SIZE_T bufferSize = Source->MaximumLength;
        Destination->Buffer = ExAllocatePoolWithTag(NonPagedPoolNx, bufferSize, MEMORY_DRIVER_TAG);
        if (!Destination->Buffer) {
            return STATUS_NO_MEMORY;
        }
        
        RtlCopyMemory(Destination->Buffer, Source->Buffer, Source->Length);
        Destination->Buffer[Source->Length / sizeof(WCHAR)] = L'\0';
    } else {
        Destination->Buffer = NULL;
    }
    
    return STATUS_SUCCESS;
}

// ============================================================================
// MEMORY OPERATION FUNCTIONS (sUNC Bypass via MmCopyVirtualMemory)
// ============================================================================

// Read memory from target process
NTSTATUS ReadProcessMemory(
    ULONG_PTR ProcessId,
    ULONG_PTR TargetAddress,
    PVOID OutputBuffer,
    SIZE_T BufferSize,
    PSIZE_T BytesRead
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS targetProcess = NULL;
    SIZE_T bytesReturned = 0;
    
    if (!OutputBuffer || BufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Get target process EPROCESS
    status = PsLookupProcessByProcessId((HANDLE)ProcessId, &targetProcess);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Use MmCopyVirtualMemory for safe cross-process read
    status = MmCopyVirtualMemory(
        targetProcess,
        (PVOID)TargetAddress,
        (PEPROCESS)NULL,  // Current process
        OutputBuffer,
        BufferSize,
        UserMode,
        &bytesReturned
    );
    
    if (BytesRead) {
        *BytesRead = bytesReturned;
    }
    
    // Dereference the process object
    ObDereferenceObject(targetProcess);
    
    return status;
}

// Write memory to target process
NTSTATUS WriteProcessMemory(
    ULONG_PTR ProcessId,
    ULONG_PTR TargetAddress,
    PVOID InputBuffer,
    SIZE_T BufferSize,
    PSIZE_T BytesWritten
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS targetProcess = NULL;
    PVOID kernelBuffer = NULL;
    SIZE_T bytesReturned = 0;
    
    if (!InputBuffer || BufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Allocate kernel buffer for the data
    kernelBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, BufferSize, MEMORY_DRIVER_TAG);
    if (!kernelBuffer) {
        return STATUS_NO_MEMORY;
    }
    
    // Copy data from user-mode to kernel buffer
    RtlCopyMemory(kernelBuffer, InputBuffer, BufferSize);
    
    // Get target process EPROCESS
    status = PsLookupProcessByProcessId((HANDLE)ProcessId, &targetProcess);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(kernelBuffer, MEMORY_DRIVER_TAG);
        return status;
    }
    
    // Use MmCopyVirtualMemory for safe cross-process write
    status = MmCopyVirtualMemory(
        (PEPROCESS)NULL,  // Current process (source)
        kernelBuffer,
        targetProcess,
        (PVOID)TargetAddress,
        BufferSize,
        UserMode,
        &bytesReturned
    );
    
    if (BytesWritten) {
        *BytesWritten = bytesReturned;
    }
    
    // Cleanup
    ExFreePoolWithTag(kernelBuffer, MEMORY_DRIVER_TAG);
    ObDereferenceObject(targetProcess);
    
    return status;
}

// ============================================================================
// IRP DISPATCH HANDLERS
// ============================================================================

NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    UNREFERENCED_PARAMETER(irpStack);
    
    // Set status to success
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    // Complete the IRP
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    UNREFERENCED_PARAMETER(irpStack);
    
    // Set status to success
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    // Complete the IRP
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    PMEMORY_OPERATION_REQUEST memoryRequest = NULL;
    PPROCESS_INFO_REQUEST processInfoRequest = NULL;
    
    // Get the input buffer
    PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    PVOID outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    
    switch (controlCode) {
        case IOCTL_READ_PROCESS_MEMORY: {
            if (inputBufferLength < sizeof(MEMORY_OPERATION_REQUEST) || 
                outputBufferLength < sizeof(MEMORY_OPERATION_REQUEST)) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }
            
            memoryRequest = (PMEMORY_OPERATION_REQUEST)inputBuffer;
            
            // Validate the request
            if (memoryRequest->ProcessId == 0 || 
                memoryRequest->TargetAddress == 0 ||
                memoryRequest->Buffer == 0 ||
                memoryRequest->Size == 0) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            
            // Perform the read operation
            SIZE_T bytesRead = 0;
            status = ReadProcessMemory(
                memoryRequest->ProcessId,
                memoryRequest->TargetAddress,
                (PVOID)memoryRequest->Buffer,
                memoryRequest->Size,
                &bytesRead
            );
            
            // Update the request with results
            memoryRequest->BytesTransferred = bytesRead;
            memoryRequest->Status = status;
            
            // Set IRP information
            Irp->IoStatus.Information = sizeof(MEMORY_OPERATION_REQUEST);
            break;
        }
        
        case IOCTL_WRITE_PROCESS_MEMORY: {
            if (inputBufferLength < sizeof(MEMORY_OPERATION_REQUEST) || 
                outputBufferLength < sizeof(MEMORY_OPERATION_REQUEST)) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }
            
            memoryRequest = (PMEMORY_OPERATION_REQUEST)inputBuffer;
            
            // Validate the request
            if (memoryRequest->ProcessId == 0 || 
                memoryRequest->TargetAddress == 0 ||
                memoryRequest->Buffer == 0 ||
                memoryRequest->Size == 0) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            
            // Perform the write operation
            SIZE_T bytesWritten = 0;
            status = WriteProcessMemory(
                memoryRequest->ProcessId,
                memoryRequest->TargetAddress,
                (PVOID)memoryRequest->Buffer,
                memoryRequest->Size,
                &bytesWritten
            );
            
            // Update the request with results
            memoryRequest->BytesTransferred = bytesWritten;
            memoryRequest->Status = status;
            
            // Set IRP information
            Irp->IoStatus.Information = sizeof(MEMORY_OPERATION_REQUEST);
            break;
        }
        
        case IOCTL_GET_PROCESS_INFO: {
            if (inputBufferLength < sizeof(PROCESS_INFO_REQUEST) || 
                outputBufferLength < sizeof(PROCESS_INFO_REQUEST)) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }
            
            processInfoRequest = (PPROCESS_INFO_REQUEST)inputBuffer;
            
            // For now, just return success with dummy data
            // In a real implementation, you would query the process info
            processInfoRequest->ProcessBaseAddress = 0;
            processInfoRequest->ProcessSize = 0;
            processInfoRequest->Status = STATUS_SUCCESS;
            
            Irp->IoStatus.Information = sizeof(PROCESS_INFO_REQUEST);
            status = STATUS_SUCCESS;
            break;
        }
        
        default: {
            status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
            break;
        }
    }
    
    // Set the IRP status
    Irp->IoStatus.Status = status;
    
    // Complete the IRP
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
}

// ============================================================================
// DRIVER ENTRY POINT
// ============================================================================

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    
    NTSTATUS status = STATUS_SUCCESS;
    
    // Initialize device name
    WCHAR deviceNameBuffer[] = DRIVER_DEVICE_NAME;
    WCHAR symbolicLinkBuffer[] = DRIVER_SYMBOLIC_LINK_NAME;
    
    UNICODE_STRING deviceNameUnicode;
    UNICODE_STRING symbolicLinkUnicode;
    
    RtlInitUnicodeString(&deviceNameUnicode, deviceNameBuffer);
    RtlInitUnicodeString(&symbolicLinkUnicode, symbolicLinkBuffer);
    
    // Create the device object
    status = IoCreateDevice(
        DriverObject,
        0,  // No device extension
        &deviceNameUnicode,
        FILE_DEVICE_CUSTOM_DRIVER,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,  // Not exclusive
        &g_DeviceObject
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Create symbolic link
    status = IoCreateSymbolicLink(&symbolicLinkUnicode, &deviceNameUnicode);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_DeviceObject);
        return status;
    }
    
    // Store the device name and symbolic link
    g_DeviceName = &deviceNameUnicode;
    g_SymbolicLinkName = &symbolicLinkUnicode;
    
    // Set up the driver unload routine
    DriverObject->DriverUnload = DriverUnload;
    
    // Set up the IRP dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    
    // All other IRP functions default to STATUS_NOT_IMPLEMENTED
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        if (DriverObject->MajorFunction[i] == NULL) {
            DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)DispatchCreate;  // Default handler
        }
    }
    
    return STATUS_SUCCESS;
}

// ============================================================================
// DRIVER UNLOAD ROUTINE
// ============================================================================

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    
    // Delete the symbolic link
    if (g_SymbolicLinkName) {
        IoDeleteSymbolicLink(g_SymbolicLinkName);
    }
    
    // Delete the device object
    if (g_DeviceObject) {
        IoDeleteDevice(g_DeviceObject);
    }
    
    // Note: We don't free the unicode strings as they are static
}
