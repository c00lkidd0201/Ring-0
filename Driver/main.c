// ============================================
// Ring-0 Driver Main File
// Windows 11 x64 | Release | No WDK
// Author: Ring-0 Driver for Roblox
// ============================================

#include "..\Include\Driver.h"

// ============================================
// Global Variables
// ============================================

PDEVICE_OBJECT g_DeviceObject = NULL;
UNICODE_STRING g_DeviceName;
UNICODE_STRING g_SymbolicLinkName;

// ============================================
// Driver Entry Point
// ============================================

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    DEBUG_LOG("DriverEntry: Initializing Ring-0 Driver");
    
    // Initialize device name and symbolic link
    RtlInitUnicodeString(&g_DeviceName, DEVICE_NAME);
    RtlInitUnicodeString(&g_SymbolicLinkName, SYMBOLIC_LINK);
    
    // Create device
    Status = CreateDevice(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        ERROR_LOG("CreateDevice failed: 0x%X", Status);
        return Status;
    }
    
    // Set up dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    
    // Set unload routine
    DriverObject->DriverUnload = DriverUnload;
    
    DEBUG_LOG("DriverEntry: Driver loaded successfully");
    DEBUG_LOG("Device: %wZ", &g_DeviceName);
    DEBUG_LOG("Symbolic Link: %wZ", &g_SymbolicLinkName);
    
    return STATUS_SUCCESS;
}

// ============================================
// Driver Unload
// ============================================

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    DEBUG_LOG("DriverUnload: Unloading Ring-0 Driver");
    
    // Delete symbolic link
    if (g_DeviceObject)
    {
        UNICODE_STRING SymbolicLink;
        RtlInitUnicodeString(&SymbolicLink, SYMBOLIC_LINK);
        IoDeleteSymbolicLink(&SymbolicLink);
    }
    
    // Delete device
    DeleteDevice(DriverObject);
    
    DEBUG_LOG("DriverUnload: Driver unloaded successfully");
}

// ============================================
// Create Device
// ============================================

NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject = NULL;
    
    DEBUG_LOG("CreateDevice: Creating device object");
    
    // Create device object
    Status = IoCreateDevice(
        DriverObject,
        0,
        &g_DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject
    );
    
    if (!NT_SUCCESS(Status))
    {
        ERROR_LOG("IoCreateDevice failed: 0x%X", Status);
        return Status;
    }
    
    // Set device flags
    DeviceObject->Flags |= DO_DIRECT_IO;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    g_DeviceObject = DeviceObject;
    
    // Create symbolic link
    Status = IoCreateSymbolicLink(&g_SymbolicLinkName, &g_DeviceName);
    if (!NT_SUCCESS(Status))
    {
        ERROR_LOG("IoCreateSymbolicLink failed: 0x%X", Status);
        DeleteDevice(DriverObject);
        return Status;
    }
    
    DEBUG_LOG("CreateDevice: Device and symbolic link created successfully");
    
    return STATUS_SUCCESS;
}

// ============================================
// Delete Device
// ============================================

VOID DeleteDevice(PDRIVER_OBJECT DriverObject)
{
    DEBUG_LOG("DeleteDevice: Deleting device object");
    
    if (g_DeviceObject)
    {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }
}

// ============================================
// Dispatch Create/Close
// ============================================

NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DEBUG_LOG("DispatchCreateClose: Handling IRP_MJ_CREATE/CLOSE");
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

// ============================================
// Dispatch Device Control
// ============================================

NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG ControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    
    DEBUG_LOG("DispatchDeviceControl: Received IOCTL: 0x%X", ControlCode);
    
    switch (ControlCode)
    {
        case IOCTL_TEST_CONNECTION:
        {
            ADDRESS_RESPONSE Response = { 0 };
            Response.Status = HandleTestConnection(&Response);
            
            Irp->IoStatus.Status = Response.Status;
            Irp->IoStatus.Information = sizeof(ADDRESS_RESPONSE);
            
            // Copy response to user buffer
            if (NT_SUCCESS(Response.Status))
            {
                PVOID UserBuffer = Irp->AssociatedIrp.SystemBuffer;
                SIZE_T BufferSize = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
                
                if (UserBuffer && BufferSize >= sizeof(ADDRESS_RESPONSE))
                {
                    __try
                    {
                        ProbeForWrite(UserBuffer, sizeof(ADDRESS_RESPONSE), sizeof(ULONG));
                        RtlCopyMemory(UserBuffer, &Response, sizeof(ADDRESS_RESPONSE));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = GetExceptionCode();
                        ERROR_LOG("Exception in IOCTL_TEST_CONNECTION: 0x%X", Status);
                    }
                }
            }
            break;
        }
        
        case IOCTL_READ_MEMORY:
        {
            PREAD_MEMORY_REQUEST Request = (PREAD_MEMORY_REQUEST)Irp->AssociatedIrp.SystemBuffer;
            SIZE_T InputBufferLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
            SIZE_T OutputBufferLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
            
            if (Request && InputBufferLength >= sizeof(READ_MEMORY_REQUEST))
            {
                PMEMORY_RESPONSE Response = (PMEMORY_RESPONSE)Irp->AssociatedIrp.SystemBuffer;
                Response->Status = HandleReadMemory(Request, Response, OutputBufferLength);
                
                Irp->IoStatus.Status = Response->Status;
                Irp->IoStatus.Information = sizeof(MEMORY_RESPONSE) + Response->BytesRead;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
                ERROR_LOG("IOCTL_READ_MEMORY: Invalid input buffer");
            }
            break;
        }
        
        case IOCTL_WRITE_MEMORY:
        {
            PWRITE_MEMORY_REQUEST Request = (PWRITE_MEMORY_REQUEST)Irp->AssociatedIrp.SystemBuffer;
            SIZE_T InputBufferLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
            
            if (Request && InputBufferLength >= sizeof(WRITE_MEMORY_REQUEST) + Request->Size)
            {
                Status = HandleWriteMemory(Request);
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
                ERROR_LOG("IOCTL_WRITE_MEMORY: Invalid input buffer");
            }
            break;
        }
        
        case IOCTL_GET_PROCESS_BASE:
        {
            PGET_PROCESS_BASE_REQUEST Request = (PGET_PROCESS_BASE_REQUEST)Irp->AssociatedIrp.SystemBuffer;
            SIZE_T InputBufferLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
            SIZE_T OutputBufferLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
            
            if (Request && InputBufferLength >= sizeof(GET_PROCESS_BASE_REQUEST))
            {
                ADDRESS_RESPONSE Response = { 0 };
                Response.Status = HandleGetProcessBase(Request, &Response);
                
                Irp->IoStatus.Status = Response.Status;
                Irp->IoStatus.Information = sizeof(ADDRESS_RESPONSE);
                
                if (NT_SUCCESS(Response.Status) && OutputBufferLength >= sizeof(ADDRESS_RESPONSE))
                {
                    PVOID UserBuffer = Irp->AssociatedIrp.SystemBuffer;
                    __try
                    {
                        ProbeForWrite(UserBuffer, sizeof(ADDRESS_RESPONSE), sizeof(ULONG));
                        RtlCopyMemory(UserBuffer, &Response, sizeof(ADDRESS_RESPONSE));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = GetExceptionCode();
                        ERROR_LOG("Exception in IOCTL_GET_PROCESS_BASE: 0x%X", Status);
                    }
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
                ERROR_LOG("IOCTL_GET_PROCESS_BASE: Invalid input buffer");
            }
            break;
        }
        
        case IOCTL_GET_MODULE_BASE:
        {
            PGET_MODULE_BASE_REQUEST Request = (PGET_MODULE_BASE_REQUEST)Irp->AssociatedIrp.SystemBuffer;
            SIZE_T InputBufferLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
            SIZE_T OutputBufferLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
            
            if (Request && InputBufferLength >= sizeof(GET_MODULE_BASE_REQUEST))
            {
                ADDRESS_RESPONSE Response = { 0 };
                Response.Status = HandleGetModuleBase(Request, &Response);
                
                Irp->IoStatus.Status = Response.Status;
                Irp->IoStatus.Information = sizeof(ADDRESS_RESPONSE);
                
                if (NT_SUCCESS(Response.Status) && OutputBufferLength >= sizeof(ADDRESS_RESPONSE))
                {
                    PVOID UserBuffer = Irp->AssociatedIrp.SystemBuffer;
                    __try
                    {
                        ProbeForWrite(UserBuffer, sizeof(ADDRESS_RESPONSE), sizeof(ULONG));
                        RtlCopyMemory(UserBuffer, &Response, sizeof(ADDRESS_RESPONSE));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = GetExceptionCode();
                        ERROR_LOG("Exception in IOCTL_GET_MODULE_BASE: 0x%X", Status);
                    }
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
                ERROR_LOG("IOCTL_GET_MODULE_BASE: Invalid input buffer");
            }
            break;
        }
        
        default:
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            ERROR_LOG("DispatchDeviceControl: Unknown IOCTL: 0x%X", ControlCode);
            break;
        }
    }
    
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
    }
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Status;
}

// ============================================
// IOCTL Handlers
// ============================================

NTSTATUS HandleTestConnection(PADDRESS_RESPONSE Response)
{
    DEBUG_LOG("HandleTestConnection: Driver is alive!");
    Response->Status = STATUS_SUCCESS;
    Response->Address = (ULONG64)NULL;
    return STATUS_SUCCESS;
}

NTSTATUS HandleReadMemory(PREAD_MEMORY_REQUEST Request, PMEMORY_RESPONSE Response, SIZE_T ResponseSize)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    DEBUG_LOG("HandleReadMemory: Address=0x%llX, Size=%zu", Request->Address, Request->Size);
    
    // Validate address
    if (!IsValidAddress((PVOID)Request->Address, Request->Size))
    {
        ERROR_LOG("HandleReadMemory: Invalid address or size");
        Response->Status = STATUS_ACCESS_VIOLATION;
        Response->BytesRead = 0;
        return STATUS_ACCESS_VIOLATION;
    }
    
    // Check response buffer size
    SIZE_T TotalResponseSize = sizeof(MEMORY_RESPONSE) + Request->Size;
    if (ResponseSize < TotalResponseSize)
    {
        ERROR_LOG("HandleReadMemory: Response buffer too small");
        Response->Status = STATUS_BUFFER_TOO_SMALL;
        Response->BytesRead = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    // Read memory
    __try
    {
        // Use MmCopyMemory for kernel-mode safe copy
        PVOID KernelBuffer = ExAllocatePoolWithTag(NonPagedPool, Request->Size, 'RING');
        if (!KernelBuffer)
        {
            ERROR_LOG("HandleReadMemory: Failed to allocate kernel buffer");
            Response->Status = STATUS_INSUFFICIENT_RESOURCES;
            Response->BytesRead = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        // Copy from target address to kernel buffer
        RtlCopyMemory(KernelBuffer, (PVOID)Request->Address, Request->Size);
        
        // Copy to response
        Response->Status = STATUS_SUCCESS;
        Response->BytesRead = Request->Size;
        RtlCopyMemory(Response->Data, KernelBuffer, Request->Size);
        
        ExFreePoolWithTag(KernelBuffer, 'RING');
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
        ERROR_LOG("HandleReadMemory: Exception: 0x%X", Status);
        Response->Status = Status;
        Response->BytesRead = 0;
    }
    
    return Status;
}

NTSTATUS HandleWriteMemory(PWRITE_MEMORY_REQUEST Request)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    DEBUG_LOG("HandleWriteMemory: Address=0x%llX, Size=%zu", Request->Address, Request->Size);
    
    // Validate address
    if (!IsValidAddress((PVOID)Request->Address, Request->Size))
    {
        ERROR_LOG("HandleWriteMemory: Invalid address or size");
        return STATUS_ACCESS_VIOLATION;
    }
    
    // Write memory
    __try
    {
        // Use MmCopyMemory for kernel-mode safe copy
        PVOID KernelBuffer = ExAllocatePoolWithTag(NonPagedPool, Request->Size, 'RING');
        if (!KernelBuffer)
        {
            ERROR_LOG("HandleWriteMemory: Failed to allocate kernel buffer");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        // Copy data from request to kernel buffer
        RtlCopyMemory(KernelBuffer, Request->Data, Request->Size);
        
        // Copy from kernel buffer to target address
        RtlCopyMemory((PVOID)Request->Address, KernelBuffer, Request->Size);
        
        ExFreePoolWithTag(KernelBuffer, 'RING');
        
        DEBUG_LOG("HandleWriteMemory: Write successful");
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
        ERROR_LOG("HandleWriteMemory: Exception: 0x%X", Status);
    }
    
    return Status;
}

NTSTATUS HandleGetProcessBase(PGET_PROCESS_BASE_REQUEST Request, PADDRESS_RESPONSE Response)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    DEBUG_LOG("HandleGetProcessBase: ProcessId=%lu", Request->ProcessId);
    
    Status = GetProcessBaseAddress(Request->ProcessId, &Response->Address);
    Response->Status = Status;
    
    if (NT_SUCCESS(Status))
    {
        DEBUG_LOG("HandleGetProcessBase: BaseAddress=0x%llX", Response->Address);
    }
    else
    {
        ERROR_LOG("HandleGetProcessBase: Failed with status 0x%X", Status);
    }
    
    return Status;
}

NTSTATUS HandleGetModuleBase(PGET_MODULE_BASE_REQUEST Request, PADDRESS_RESPONSE Response)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    DEBUG_LOG("HandleGetModuleBase: ProcessId=%lu, ModuleName=%s", Request->ProcessId, Request->ModuleName);
    
    Status = GetModuleBaseAddress(Request->ProcessId, Request->ModuleName, &Response->Address);
    Response->Status = Status;
    
    if (NT_SUCCESS(Status))
    {
        DEBUG_LOG("HandleGetModuleBase: ModuleBase=0x%llX", Response->Address);
    }
    else
    {
        ERROR_LOG("HandleGetModuleBase: Failed with status 0x%X", Status);
    }
    
    return Status;
}

// ============================================
// Memory Utilities
// ============================================

BOOLEAN IsValidAddress(PVOID Address, SIZE_T Size)
{
    if (!Address || Size == 0)
        return FALSE;
    
    // Check if address is in user space
    if ((ULONG64)Address < 0x10000 || (ULONG64)Address > 0x7FFFFFFFFFFF)
        return FALSE;
    
    // Check if address + size would overflow
    if ((ULONG64)Address + Size < (ULONG64)Address)
        return FALSE;
    
    // Check if the memory region is accessible
    // Note: This is a basic check, more sophisticated validation needed for production
    MEMORY_BASIC_INFORMATION MemoryInfo;
    NTSTATUS Status = ZwQueryVirtualMemory(
        NtCurrentProcess(),
        Address,
        MemoryBasicInformation,
        &MemoryInfo,
        sizeof(MemoryInfo),
        NULL
    );
    
    if (!NT_SUCCESS(Status))
        return FALSE;
    
    // Check if the region is committed and readable
    if (MemoryInfo.State != MEM_COMMIT || 
        (MemoryInfo.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) == 0)
        return FALSE;
    
    return TRUE;
}

NTSTATUS SafeCopyMemory(PVOID Destination, PVOID Source, SIZE_T Size)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    __try
    {
        ProbeForRead(Source, Size, sizeof(UCHAR));
        ProbeForWrite(Destination, Size, sizeof(UCHAR));
        RtlCopyMemory(Destination, Source, Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    
    return Status;
}

// ============================================
// Process Utilities
// ============================================

NTSTATUS GetProcessBaseAddress(ULONG ProcessId, PULONG64 BaseAddress)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process = NULL;
    
    *BaseAddress = 0;
    
    // Get process object
    Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &Process);
    if (!NT_SUCCESS(Status))
    {
        ERROR_LOG("GetProcessBaseAddress: PsLookupProcessByProcessId failed: 0x%X", Status);
        return Status;
    }
    
    // Get process base address
    *BaseAddress = (ULONG64)PsGetProcessSectionBaseAddress(Process);
    
    // Release process object
    ObDereferenceObject(Process);
    
    if (*BaseAddress == 0)
    {
        ERROR_LOG("GetProcessBaseAddress: Failed to get base address");
        return STATUS_UNSUCCESSFUL;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS GetModuleBaseAddress(ULONG ProcessId, PCSTR ModuleName, PULONG64 BaseAddress)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process = NULL;
    KAPC_STATE ApcState;
    
    *BaseAddress = 0;
    
    // Get process object
    Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &Process);
    if (!NT_SUCCESS(Status))
    {
        ERROR_LOG("GetModuleBaseAddress: PsLookupProcessByProcessId failed: 0x%X", Status);
        return Status;
    }
    
    // Attach to process
    KeStackAttachProcess(Process, &ApcState);
    
    __try
    {
        // Get PEB
        PPEB Peb = PsGetProcessPeb(Process);
        if (!Peb)
        {
            Status = STATUS_UNSUCCESSFUL;
            __leave;
        }
        
        // Walk the module list
        PLIST_ENTRY ModuleList = &Peb->Ldr->InMemoryOrderModuleList;
        PLDR_DATA_TABLE_ENTRY ModuleEntry = NULL;
        
        for (PLIST_ENTRY Entry = ModuleList->Flink; Entry != ModuleList; Entry = Entry->Flink)
        {
            ModuleEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
            
            if (ModuleEntry->BaseDllName.Buffer)
            {
                CHAR ModuleNameBuffer[256] = { 0 };
                SIZE_T BytesCopied = 0;
                
                // Convert UNICODE_STRING to ANSI
                Status = RtlUnicodeStringToAnsiStringN(
                    (PANSI_STRING)ModuleNameBuffer,
                    sizeof(ModuleNameBuffer) - 1,
                    &BytesCopied,
                    &ModuleEntry->BaseDllName,
                    FALSE
                );
                
                if (NT_SUCCESS(Status))
                {
                    // Compare module names (case insensitive)
                    if (_stricmp(ModuleNameBuffer, ModuleName) == 0)
                    {
                        *BaseAddress = (ULONG64)ModuleEntry->DllBase;
                        break;
                    }
                }
            }
        }
        
        if (*BaseAddress == 0)
        {
            Status = STATUS_NOT_FOUND;
        }
    }
    __finally
    {
        // Detach from process
        KeUnstackDetachProcess(&ApcState);
        ObDereferenceObject(Process);
    }
    
    return Status;
}
