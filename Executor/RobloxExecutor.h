#pragma once

// ============================================
// Roblox Executor DLL Header
// Windows 11 x64 | Release
// ============================================

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

// ============================================
// Driver Communication
// ============================================

#define DEVICE_NAME L"\\\\.\\Ring0Driver"

// IO Control Codes (must match driver)
#define IOCTL_BASE 0x8000

#define IOCTL_READ_MEMORY CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x1,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

#define IOCTL_WRITE_MEMORY CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x2,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

#define IOCTL_GET_PROCESS_BASE CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x3,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

#define IOCTL_GET_MODULE_BASE CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x4,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

#define IOCTL_TEST_CONNECTION CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    IOCTL_BASE + 0x5,
    METHOD_BUFFERED,
    FILE_ANY_ACCESS
)

// ============================================
// Request/Response Structures (must match driver)
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
// Driver Handle
// ============================================

class DriverHandle {
public:
    DriverHandle();
    ~DriverHandle();
    
    bool Open();
    void Close();
    bool IsOpen() const;
    
    bool TestConnection();
    bool ReadMemory(ULONG64 Address, SIZE_T Size, LPVOID Buffer);
    bool WriteMemory(ULONG64 Address, SIZE_T Size, LPCVOID Buffer);
    bool GetProcessBaseAddress(ULONG ProcessId, ULONG64* BaseAddress);
    bool GetModuleBaseAddress(ULONG ProcessId, LPCSTR ModuleName, ULONG64* BaseAddress);
    
    DWORD GetLastError() const;
    
private:
    HANDLE m_hDevice;
    DWORD m_dwLastError;
};

// ============================================
// Roblox Process Utilities
// ============================================

class RobloxProcess {
public:
    RobloxProcess();
    ~RobloxProcess();
    
    bool Attach(LPCWSTR ProcessName = L"RobloxPlayerBeta.exe");
    void Detach();
    bool IsAttached() const;
    
    DWORD GetProcessId() const;
    HANDLE GetProcessHandle() const;
    ULONG64 GetBaseAddress() const;
    ULONG64 GetModuleBaseAddress(LPCSTR ModuleName);
    
    bool ReadMemory(ULONG64 Address, SIZE_T Size, LPVOID Buffer);
    bool WriteMemory(ULONG64 Address, SIZE_T Size, LPCVOID Buffer);
    
    template<typename T>
    bool Read(ULONG64 Address, T* Value) {
        return ReadMemory(Address, sizeof(T), Value);
    }
    
    template<typename T>
    bool Write(ULONG64 Address, T Value) {
        return WriteMemory(Address, sizeof(T), &Value);
    }
    
private:
    DWORD m_dwProcessId;
    HANDLE m_hProcess;
    ULONG64 m_ullBaseAddress;
    DriverHandle m_Driver;
};

// ============================================
// Roblox Memory Utilities
// ============================================

class RobloxMemory {
public:
    static bool FindPattern(ULONG64 StartAddress, SIZE_T Size, const char* Pattern, const char* Mask, ULONG64* FoundAddress);
    static bool FindSignature(const RobloxProcess& Process, const char* ModuleName, const char* Pattern, const char* Mask, ULONG64* FoundAddress);
    static ULONG64 ResolvePointerChain(const RobloxProcess& Process, ULONG64 BaseAddress, const std::vector<ULONG64>& Offsets);
    
    // Roblox-specific patterns
    static bool FindDataModel(const RobloxProcess& Process, ULONG64* DataModelAddress);
    static bool FindGameObject(const RobloxProcess& Process, ULONG64* GameObjectAddress);
};

// ============================================
// Lua Execution
// ============================================

class LuaExecutor {
public:
    LuaExecutor();
    ~LuaExecutor();
    
    bool Initialize(const RobloxProcess& Process);
    bool ExecuteScript(const char* Script);
    bool ExecuteBytecode(const unsigned char* Bytecode, SIZE_T Size);
    
private:
    RobloxProcess* m_pProcess;
    ULONG64 m_ullLuaState;
    ULONG64 m_ullLuaLoad;
    ULONG64 m_ullLuaPCall;
};

// ============================================
// Export Functions
// ============================================

#ifdef ROBLOXEXECUTOR_EXPORTS
#define ROBLOX_API __declspec(dllexport)
#else
#define ROBLOX_API __declspec(dllimport)
#endif

extern "C" {
    ROBLOX_API bool __cdecl InitializeExecutor();
    ROBLOX_API bool __cdecl ExecuteLuaScript(const char* Script);
    ROBLOX_API bool __cdecl ExecuteLuaBytecode(const unsigned char* Bytecode, SIZE_T Size);
    ROBLOX_API void __cdecl ShutdownExecutor();
    ROBLOX_API bool __cdecl IsDriverLoaded();
    ROBLOX_API bool __cdecl TestDriverConnection();
}
