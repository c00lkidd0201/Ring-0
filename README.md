# Ring-0 Driver & Roblox Executor

## 📋 Обзор

Этот проект содержит **Ring-0 Driver** для Windows 11 x64 и **Executor DLL** для Roblox, который позволяет выполнять Lua скрипты и байткод через драйвер.

### 🎯 Возможности

- **Ring-0 Driver**: Работает на уровне ядра, предоставляет доступ к:
  - Чтение/запись памяти процессов
  - Получение базового адреса процесса
  - Получение базового адреса модулей
  - Тестирование соединения с драйвером

- **Executor DLL**: Взаимодействует с драйвером для:
  - Подключения к Roblox процессу
  - Выполнения Lua скриптов
  - Выполнения Lua байткода
  - Поиска паттернов в памяти

## 🛠 Сборка

### Требования

- **Visual Studio 2022** (или новее)
- **Windows 11 SDK** (включен в Visual Studio)
- **Windows 11 x64** (для запуска)

### Сборка драйвера

1. Откройте решение `Ring0.sln` в Visual Studio
2. Выберите конфигурацию **Release** и платформу **x64**
3. Соберите проект **Driver**
4. Готовый драйвер будет в папке `Release\Driver\Ring0Driver.sys`

### Сборка Executor DLL

1. В том же решении выберите проект **Executor**
2. Соберите проект
3. Готовая DLL будет в папке `Release\Executor\RobloxExecutor.dll`

## 🚀 Запуск

### 1. Загрузка драйвера через kdmapper

```cmd
kdmapper_Release.exe Ring0Driver.sys
```

**Примечание**: kdmapper должен быть запущен от имени администратора.

### 2. Проверка работы драйвера

Можно проверить через **DebugView** или с помощью тестового кода:

```cpp
#include "RobloxExecutor.h"

int main() {
    if (IsDriverLoaded()) {
        printf("Driver is loaded!\n");
        if (TestDriverConnection()) {
            printf("Driver connection test: SUCCESS\n");
        } else {
            printf("Driver connection test: FAILED\n");
        }
    } else {
        printf("Driver is not loaded\n");
    }
    return 0;
}
```

### 3. Использование Executor

```cpp
#include "RobloxExecutor.h"

int main() {
    // Инициализация
    if (!InitializeExecutor()) {
        printf("Failed to initialize executor\n");
        return 1;
    }
    
    // Выполнение Lua скрипта
    const char* script = "print('Hello from Ring-0!')";
    if (ExecuteLuaScript(script)) {
        printf("Script executed successfully\n");
    } else {
        printf("Failed to execute script\n");
    }
    
    // Завершение работы
    ShutdownExecutor();
    return 0;
}
```

## 📊 IOCTL Коды

| IOCTL | Описание |
|-------|----------|
| `0x8000` | Тест соединения |
| `0x8001` | Чтение памяти |
| `0x8002` | Запись памяти |
| `0x8003` | Получение базового адреса процесса |
| `0x8004` | Получение базового адреса модуля |

## 🔍 Отладка

### Просмотр логов в DebugView

1. Запустите **DebugView** от имени администратора
2. Установите фильтр на `[Ring0]` и `[Executor]`
3. Запустите драйвер и executor
4. Все сообщения будут отображаться в DebugView

### Распространенные ошибки

| Ошибка | Причина | Решение |
|-------|---------|---------|
| `STATUS_ACCESS_DENIED` | Недостаточно прав | Запустите от администратора |
| `STATUS_DEVICE_NOT_FOUND` | Драйвер не загружен | Загрузите драйвер через kdmapper |
| `STATUS_INVALID_PARAMETER` | Неверные параметры | Проверьте параметры вызова |
| `STATUS_ACCESS_VIOLATION` | Недопустимый адрес памяти | Проверьте адреса памяти |
| `STATUS_BUFFER_TOO_SMALL` | Маленький буфер | Увеличьте размер буфера |

## 🛡 Защита от BSOD

Драйвер включает несколько механизмов защиты:

1. **Проверка адресов**: `IsValidAddress()` проверяет допустимость адресов памяти
2. **Исключения**: Используются `__try/__except` блоки для перехвата исключений
3. **Проверка буферов**: Проверка размеров буферов перед копированием
4. **Логгирование**: Все ошибки логируются в DebugView

## 🎮 Roblox Version 9affbe66b2624d20

Для работы с этой версией Roblox необходимо:

1. Обновить паттерны в `RobloxMemory::FindDataModel()` и `RobloxMemory::FindGameObject()`
2. Найти адреса Lua функций (lua_state, lua_load, lua_pcall)
3. Реализовать выполнение Lua скриптов

### Пример паттернов для Roblox

```cpp
// Patтерн для DataModel
const char* DataModelPattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x40\x00";
const char* DataModelMask = "xxx????xxxx?xxx?";

// Patтерн для Lua State
const char* LuaStatePattern = "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0";
const char* LuaStateMask = "xxx????x????xxx";
```

## 📁 Структура проекта

```
Ring-0/
├── Driver/
│   ├── main.c          # Основной файл драйвера
│   ├── Driver.vcxproj  # Проект Visual Studio
│   └── Driver.def      # Декларация экспортов
├── Executor/
│   ├── RobloxExecutor.cpp  # Реализация Executor
│   ├── RobloxExecutor.h    # Заголовочный файл
│   └── Executor.vcxproj    # Проект Visual Studio
├── Include/
│   └── Driver.h       # Общие определения
├── Ring0.sln           # Решение Visual Studio
└── README.md           # Документация
```

## ⚠ Важно

1. **Безопасность**: Драйвер работает на уровне ядра. Неправильное использование может привести к BSOD.
2. **Античит**: Использование таких инструментов может нарушать правила Roblox и приводить к бану.
3. **Легальность**: Проверьте местное законодательство перед использованием.
4. **Отладка**: Всегда тестируйте в виртуальной машине перед использованием на основной системе.

## 🔧 Технические детали

### Драйвер

- **Тип**: WDM Driver (Windows Driver Model)
- **Платформа**: x64
- **Конфигурация**: Release
- **Зависимости**: ntoskrnl.lib, hal.lib
- **Точка входа**: DriverEntry

### Executor DLL

- **Тип**: Dynamic Link Library
- **Платформа**: x64
- **Конфигурация**: Release
- **Экспорты**: InitializeExecutor, ExecuteLuaScript, ExecuteLuaBytecode, ShutdownExecutor, IsDriverLoaded, TestDriverConnection
- **Зависимости**: kernel32.lib, user32.lib, Psapi.lib

## 📞 Поддержка

Если у вас есть вопросы или проблемы:

1. Проверьте логи в DebugView
2. Убедитесь, что драйвер загружен
3. Проверьте права доступа
4. Проверьте параметры вызова IOCTL

## 🎉 Пример использования

```cpp
#include <Windows.h>
#include <stdio.h>

// Импортируем функции из DLL
typedef bool (__cdecl *PFN_INITIALIZE_EXECUTOR)();
typedef bool (__cdecl *PFN_EXECUTE_LUA_SCRIPT)(const char*);
typedef void (__cdecl *PFN_SHUTDOWN_EXECUTOR)();

int main() {
    HMODULE hDll = LoadLibraryA("RobloxExecutor.dll");
    if (!hDll) {
        printf("Failed to load DLL\n");
        return 1;
    }
    
    auto InitializeExecutor = (PFN_INITIALIZE_EXECUTOR)GetProcAddress(hDll, "InitializeExecutor");
    auto ExecuteLuaScript = (PFN_EXECUTE_LUA_SCRIPT)GetProcAddress(hDll, "ExecuteLuaScript");
    auto ShutdownExecutor = (PFN_SHUTDOWN_EXECUTOR)GetProcAddress(hDll, "ShutdownExecutor");
    
    if (!InitializeExecutor || !ExecuteLuaScript || !ShutdownExecutor) {
        printf("Failed to get function pointers\n");
        FreeLibrary(hDll);
        return 1;
    }
    
    if (InitializeExecutor()) {
        printf("Executor initialized\n");
        
        const char* script = "game.Players.LocalPlayer.Character.Humanoid.WalkSpeed = 50";
        if (ExecuteLuaScript(script)) {
            printf("Script executed!\n");
        } else {
            printf("Failed to execute script\n");
        }
        
        ShutdownExecutor();
    } else {
        printf("Failed to initialize executor\n");
    }
    
    FreeLibrary(hDll);
    return 0;
}
```
