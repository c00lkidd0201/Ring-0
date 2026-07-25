// dllmain.cpp
#pragma pack(push, 8)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include "SharedDefs.h"

// --- Глобальная переменная для хендла устройства ---
HANDLE g_DeviceHandle = INVALID_HANDLE_VALUE;
HANDLE g_HeapHandle = NULL;

// --- Инициализация кучи ---
BOOL InitializeHeap() {
    g_HeapHandle = HeapCreate(0, 0, 0);
    return (g_HeapHandle != NULL);
}

// --- Освобождение кучи ---
VOID FreeHeap() {
    if (g_HeapHandle) {
        HeapDestroy(g_HeapHandle);
        g_HeapHandle = NULL;
    }
}

// --- Инициализация соединения с драйвером ---
BOOL InitializeDriverConnection() {
    g_DeviceHandle = CreateFileW(
        L"\\.\\MemoryDriver",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    return (g_DeviceHandle != INVALID_HANDLE_VALUE);
}

// --- Закрытие соединения с драйвером ---
VOID CloseDriverConnection() {
    if (g_DeviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_DeviceHandle);
        g_DeviceHandle = INVALID_HANDLE_VALUE;
    }
}

// --- Чтение памяти через драйвер ---
BOOL ReadMemory(
    DWORD ProcessId,
    ULONG64 Address,
    PVOID Buffer,
    SIZE_T Size
) {
    if (g_DeviceHandle == INVALID_HANDLE_VALUE || Buffer == nullptr || Size == 0) {
        return FALSE;
    }

    SIZE_T RequestSize = sizeof(MEMORY_READ_REQUEST) + Size - 1;
    PMEMORY_READ_REQUEST Request = (PMEMORY_READ_REQUEST)HeapAlloc(g_HeapHandle, 0, RequestSize);
    if (!Request) {
        return FALSE;
    }

    Request->ProcessId = ProcessId;
    Request->Address = Address;
    Request->Size = (unsigned long)Size;

    DWORD BytesReturned = 0;
    BOOL Result = DeviceIoControl(
        g_DeviceHandle,
        IOCTL_READ_MEMORY,
        Request,
        (DWORD)RequestSize,
        Buffer,
        (DWORD)Size,
        &BytesReturned,
        NULL
    );

    HeapFree(g_HeapHandle, 0, Request);
    return Result;
}

// --- Запись памяти через драйвер ---
BOOL WriteMemory(
    DWORD ProcessId,
    ULONG64 Address,
    PVOID Data,
    SIZE_T Size
) {
    if (g_DeviceHandle == INVALID_HANDLE_VALUE || Data == nullptr || Size == 0) {
        return FALSE;
    }

    SIZE_T RequestSize = sizeof(MEMORY_WRITE_REQUEST) + Size - 1;
    PMEMORY_WRITE_REQUEST Request = (PMEMORY_WRITE_REQUEST)HeapAlloc(g_HeapHandle, 0, RequestSize);
    if (!Request) {
        return FALSE;
    }

    Request->ProcessId = ProcessId;
    Request->Address = Address;
    Request->Size = (unsigned long)Size;
    memcpy(Request->Data, Data, Size);

    DWORD BytesReturned = 0;
    BOOL Result = DeviceIoControl(
        g_DeviceHandle,
        IOCTL_WRITE_MEMORY,
        Request,
        (DWORD)RequestSize,
        NULL,
        0,
        &BytesReturned,
        NULL
    );

    HeapFree(g_HeapHandle, 0, Request);
    return Result;
}

// --- Точка входа DLL ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    (void)hModule;
    (void)lpReserved;

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            if (!InitializeHeap() || !InitializeDriverConnection()) {
                FreeHeap();
                return FALSE;
            }
            break;
        case DLL_PROCESS_DETACH:
            CloseDriverConnection();
            FreeHeap();
            break;
    }
    return TRUE;
}

#pragma pack(pop)
