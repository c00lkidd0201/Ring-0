// SharedDefs.h
#pragma once

// --- Принудительное выравнивание ---
#if defined(_KERNEL_MODE)
#pragma pack(push, 1)
#else
#pragma pack(push, 8)
#endif

// --- IOCTL команды (только если не определены в winioctl.h) ---
#ifndef CTL_CODE
#define FILE_ANY_ACCESS      0x00000000
#define FILE_SPECIAL_ACCESS  0x00000000
#define METHOD_BUFFERED      0x00000000
#define FILE_DEVICE_UNKNOWN  0x00000022

#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

#define IOCTL_READ_MEMORY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)

// --- Структуры для обмена данными ---
typedef struct _MEMORY_READ_REQUEST {
    unsigned long ProcessId;
    unsigned long long Address;
    unsigned long Size;
    unsigned char Buffer[1];
} MEMORY_READ_REQUEST, *PMEMORY_READ_REQUEST;

typedef struct _MEMORY_WRITE_REQUEST {
    unsigned long ProcessId;
    unsigned long long Address;
    unsigned long Size;
    unsigned char Data[1];
} MEMORY_WRITE_REQUEST, *PMEMORY_WRITE_REQUEST;

#pragma pack(pop)
