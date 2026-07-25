// SharedDefs.h
#pragma once

// --- Принудительное выравнивание для совместимости с /Zp1 (драйвер) и /Zp8 (DLL) ---
#if defined(_KERNEL_MODE)
#pragma pack(push, 1) // Выравнивание 1 байт для ядра
#else
#pragma pack(push, 8) // Выравнивание 8 байт для user-mode
#endif

// --- Ручное определение CTL_CODE ---
#define FILE_ANY_ACCESS      0x00000000
#define FILE_SPECIAL_ACCESS  0x00000000
#define METHOD_BUFFERED      0x00000000
#define FILE_DEVICE_UNKNOWN  0x00000022

#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

// --- IOCTL команды ---
#define IOCTL_READ_MEMORY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)

// --- Структуры для обмена данными ---
typedef struct _MEMORY_READ_REQUEST {
    unsigned long ProcessId;      // PID целевого процесса
    unsigned long long Address;   // Адрес для чтения
    unsigned long Size;           // Размер буфера
    unsigned char Buffer[1];      // Гибкий массив для данных
} MEMORY_READ_REQUEST, *PMEMORY_READ_REQUEST;

typedef struct _MEMORY_WRITE_REQUEST {
    unsigned long ProcessId;      // PID целевого процесса
    unsigned long long Address;   // Адрес для записи
    unsigned long Size;           // Размер данных
    unsigned char Data[1];        // Гибкий массив для данных
} MEMORY_WRITE_REQUEST, *PMEMORY_WRITE_REQUEST;

#pragma pack(pop) // Восстановление выравнивания
