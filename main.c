// main.c
#pragma pack(push, 1)

// --- Базовые типы для NO-WDK ---
typedef void* PVOID;
typedef unsigned long ULONG;
typedef unsigned long long ULONG64;
typedef long NTSTATUS;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef int BOOLEAN;

// --- NULL и SIZE_T ---
#ifndef NULL
#define NULL ((void*)0)
#endif

typedef ULONG64 SIZE_T;

// --- wchar_t (если не определен) ---
#ifndef _WCHAR_T_DEFINED
typedef wchar_t WCHAR;
#define _WCHAR_T_DEFINED
#endif

// --- NTSTATUS коды ---
#define STATUS_SUCCESS              ((NTSTATUS)0x00000000)
#define STATUS_UNSUCCESSFUL        ((NTSTATUS)0xC0000001)
#define STATUS_INVALID_PARAMETER   ((NTSTATUS)0xC000000D)

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

// --- Структуры ядра ---
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    WCHAR* Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _DRIVER_OBJECT {
    USHORT Type;
    USHORT Size;
    PVOID DeviceObject;
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
    PVOID DriverSection;
    PVOID DriverExtension;
    UNICODE_STRING DriverName;
    PVOID* HardwareDatabase;
    PVOID FastIoDispatch;
    PVOID DriverInit;
    PVOID DriverStartIo;
    PVOID DriverUnload;
    PVOID MajorFunction[28];
} DRIVER_OBJECT, *PDRIVER_OBJECT;

typedef struct _DEVICE_OBJECT {
    USHORT Type;
    USHORT Size;
    PVOID DeviceExtension;
    PDRIVER_OBJECT DriverObject;
    PVOID NextDevice;
    UNICODE_STRING DeviceName;
    UNICODE_STRING DriverName;
    PVOID DeviceQueue;
    ULONG DeviceType;
    ULONG Characteristics;
    ULONG ReferenceCount;
    ULONG Flags;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

typedef struct _IRP {
    USHORT Type;
    USHORT Size;
    PVOID MdlAddress;
    ULONG Flags;
    PVOID AssociatedIrp;
    PVOID ThreadListEntry;
    PVOID IoStatus;
    PVOID RequestorMode;
    PVOID PendingReturned;
    PVOID StackCount;
    PVOID CurrentLocation;
    PVOID Cancel;
    PVOID CancelIrp;
    PVOID ApcEnvironment;
    PVOID AllocationControl;
    PVOID UserIosb;
    PVOID UserEvent;
    PVOID Overlay;
    PVOID CancelRoutine;
    PVOID UserBuffer;
} IRP, *PIRP;

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;
    PVOID Parameters;
    PVOID DeviceObject;
    PVOID FileObject;
    PVOID CompletionRoutine;
    PVOID Context;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

// --- Прототипы функций ядра ---
__declspec(dllimport) NTSTATUS IoCreateDevice(
    PDRIVER_OBJECT DriverObject,
    ULONG DeviceExtensionSize,
    PUNICODE_STRING DeviceName,
    ULONG DeviceType,
    ULONG DeviceCharacteristics,
    BOOLEAN Exclusive,
    PDEVICE_OBJECT* DeviceObject
);

__declspec(dllimport) NTSTATUS IoCreateSymbolicLink(
    PUNICODE_STRING SymbolicLinkName,
    PUNICODE_STRING DeviceName
);

__declspec(dllimport) VOID IoDeleteDevice(
    PDEVICE_OBJECT DeviceObject
);

__declspec(dllimport) NTSTATUS IoDeleteSymbolicLink(
    PUNICODE_STRING SymbolicLinkName
);

__declspec(dllimport) VOID IoCompleteRequest(
    PIRP Irp,
    UCHAR PriorityBoost
);

__declspec(dllimport) NTSTATUS MmCopyVirtualMemory(
    PVOID SourceProcess,
    PVOID SourceAddress,
    PVOID TargetProcess,
    PVOID TargetAddress,
    SIZE_T Size,
    ULONG Flags,
    PSIZE_T ReturnSize
);

// --- IOCTL команды ---
#define FILE_ANY_ACCESS      0x00000000
#define METHOD_BUFFERED      0x00000000
#define FILE_DEVICE_UNKNOWN  0x00000022
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#define IOCTL_READ_MEMORY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)

// --- Структуры для обмена данными ---
typedef struct _MEMORY_READ_REQUEST {
    ULONG ProcessId;
    ULONG64 Address;
    ULONG Size;
    UCHAR Buffer[1];
} MEMORY_READ_REQUEST, *PMEMORY_READ_REQUEST;

typedef struct _MEMORY_WRITE_REQUEST {
    ULONG ProcessId;
    ULONG64 Address;
    ULONG Size;
    UCHAR Data[1];
} MEMORY_WRITE_REQUEST, *PMEMORY_WRITE_REQUEST;

// --- Глобальные переменные ---
PDEVICE_OBJECT g_DeviceObject = NULL;
UNICODE_STRING g_DeviceName;
UNICODE_STRING g_SymbolicLinkName;

// --- Вспомогательные функции ---
NTSTATUS InitializeUnicodeString(PUNICODE_STRING String, const WCHAR* Buffer) {
    String->Buffer = (WCHAR*)Buffer;
    String->Length = (USHORT)(wcslen(Buffer) * sizeof(WCHAR));
    String->MaximumLength = String->Length + sizeof(WCHAR);
    return STATUS_SUCCESS;
}

// --- Обработчик IOCTL ---
NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IoStack = (PIO_STACK_LOCATION)Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    ULONG BytesReturned = 0;

    if (!IoStack) {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, 0);
        return Status;
    }

    PVOID InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;

    if (IoControlCode == IOCTL_READ_MEMORY) {
        PMEMORY_READ_REQUEST Request = (PMEMORY_READ_REQUEST)InputBuffer;
        if (Request && Request->Size > 0) {
            SIZE_T ReturnSize = 0;
            Status = MmCopyVirtualMemory(
                (PVOID)(ULONG64)Request->ProcessId,
                (PVOID)Request->Address,
                (PVOID)(-1),
                Request->Buffer,
                Request->Size,
                0,
                &ReturnSize
            );
            if (NT_SUCCESS(Status)) {
                BytesReturned = (ULONG)ReturnSize;
            }
        }
    }
    else if (IoControlCode == IOCTL_WRITE_MEMORY) {
        PMEMORY_WRITE_REQUEST Request = (PMEMORY_WRITE_REQUEST)InputBuffer;
        if (Request && Request->Size > 0) {
            SIZE_T ReturnSize = 0;
            Status = MmCopyVirtualMemory(
                (PVOID)(-1),
                Request->Data,
                (PVOID)(ULONG64)Request->ProcessId,
                (PVOID)Request->Address,
                Request->Size,
                0,
                &ReturnSize
            );
            if (NT_SUCCESS(Status)) {
                BytesReturned = (ULONG)ReturnSize;
            }
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesReturned;
    IoCompleteRequest(Irp, 0);
    return Status;
}

// --- Обработчик создания устройства ---
NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    (void)DeviceObject;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;
}

// --- Обработчик закрытия устройства ---
NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    (void)DeviceObject;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;
}

// --- Функция выгрузки драйвера ---
VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    (void)DriverObject;
    if (g_DeviceObject) {
        IoDeleteSymbolicLink(&g_SymbolicLinkName);
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }
}

// --- Точка входа драйвера ---
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    (void)RegistryPath;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    static WCHAR DeviceNameBuffer[] = L"\\Device\\MemoryDriver";
    static WCHAR SymbolicLinkBuffer[] = L"\\DosDevices\\MemoryDriver";

    InitializeUnicodeString(&g_DeviceName, DeviceNameBuffer);
    InitializeUnicodeString(&g_SymbolicLinkName, SymbolicLinkBuffer);

    Status = IoCreateDevice(
        DriverObject,
        0,
        &g_DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = IoCreateSymbolicLink(&g_SymbolicLinkName, &g_DeviceName);
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return Status;
    }

    DriverObject->MajorFunction[0x00] = (PVOID)DispatchCreate;
    DriverObject->MajorFunction[0x04] = (PVOID)DispatchClose;
    DriverObject->MajorFunction[0x0E] = (PVOID)DispatchDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}

#pragma pack(pop)
