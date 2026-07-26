// ============================================
// Roblox Executor DLL Implementation
// Windows 11 x64 | Release
// ============================================

#include "RobloxExecutor.h"
#include <vector>
#include <string>
#include <Psapi.h>
#include <TlHelp32.h>

#pragma comment(lib, "Psapi.lib")

// ============================================
// Debug Logging
// ============================================

#define EXECUTOR_PREFIX "[Executor] "

#ifdef _DEBUG
#define DEBUG_LOG(fmt, ...) OutputDebugStringA(EXECUTOR_PREFIX fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif

#define ERROR_LOG(fmt, ...) OutputDebugStringA(EXECUTOR_PREFIX "ERROR: " fmt "\n", ##__VA_ARGS__)

// ============================================
// DriverHandle Implementation
// ============================================

DriverHandle::DriverHandle() : m_hDevice(INVALID_HANDLE_VALUE), m_dwLastError(ERROR_SUCCESS) {}

DriverHandle::~DriverHandle() {
    Close();
}

bool DriverHandle::Open() {
    Close();
    
    DEBUG_LOG("Opening driver device: %ws", DEVICE_NAME);
    
    m_hDevice = CreateFileW(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (m_hDevice == INVALID_HANDLE_VALUE) {
        m_dwLastError = GetLastError();
        ERROR_LOG("Failed to open driver: %lu", m_dwLastError);
        return false;
    }
    
    DEBUG_LOG("Driver device opened successfully");
    return true;
}

void DriverHandle::Close() {
    if (m_hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hDevice);
        m_hDevice = INVALID_HANDLE_VALUE;
        DEBUG_LOG("Driver device closed");
    }
}

bool DriverHandle::IsOpen() const {
    return m_hDevice != INVALID_HANDLE_VALUE;
}

DWORD DriverHandle::GetLastError() const {
    return m_dwLastError;
}

bool DriverHandle::TestConnection() {
    if (!IsOpen()) {
        ERROR_LOG("Driver not open");
        return false;
    }
    
    ADDRESS_RESPONSE Response = { 0 };
    DWORD BytesReturned = 0;
    
    bool Result = DeviceIoControl(
        m_hDevice,
        IOCTL_TEST_CONNECTION,
        NULL,
        0,
        &Response,
        sizeof(Response),
        &BytesReturned,
        NULL
    );
    
    if (!Result) {
        m_dwLastError = GetLastError();
        ERROR_LOG("TestConnection failed: %lu", m_dwLastError);
        return false;
    }
    
    if (!NT_SUCCESS(Response.Status)) {
        ERROR_LOG("TestConnection: Driver returned error: 0x%X", Response.Status);
        return false;
    }
    
    DEBUG_LOG("TestConnection: Driver is alive!");
    return true;
}

bool DriverHandle::ReadMemory(ULONG64 Address, SIZE_T Size, LPVOID Buffer) {
    if (!IsOpen() || !Buffer || Size == 0) {
        ERROR_LOG("Invalid parameters for ReadMemory");
        return false;
    }
    
    // Allocate buffer for request and response
    SIZE_T RequestSize = sizeof(READ_MEMORY_REQUEST);
    SIZE_T ResponseSize = sizeof(MEMORY_RESPONSE) + Size;
    
    PVOID pBuffer = VirtualAlloc(NULL, max(RequestSize, ResponseSize), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pBuffer) {
        ERROR_LOG("Failed to allocate memory for ReadMemory");
        return false;
    }
    
    PREAD_MEMORY_REQUEST pRequest = (PREAD_MEMORY_REQUEST)pBuffer;
    PMEMORY_RESPONSE pResponse = (PMEMORY_RESPONSE)pBuffer;
    
    pRequest->Address = Address;
    pRequest->Size = Size;
    
    DWORD BytesReturned = 0;
    bool Result = DeviceIoControl(
        m_hDevice,
        IOCTL_READ_MEMORY,
        pRequest,
        RequestSize,
        pResponse,
        ResponseSize,
        &BytesReturned,
        NULL
    );
    
    if (!Result) {
        m_dwLastError = GetLastError();
        ERROR_LOG("ReadMemory DeviceIoControl failed: %lu", m_dwLastError);
        VirtualFree(pBuffer, 0, MEM_RELEASE);
        return false;
    }
    
    if (!NT_SUCCESS(pResponse->Status)) {
        ERROR_LOG("ReadMemory: Driver returned error: 0x%X", pResponse->Status);
        VirtualFree(pBuffer, 0, MEM_RELEASE);
        return false;
    }
    
    // Copy data to output buffer
    if (pResponse->BytesRead > 0 && pResponse->BytesRead <= Size) {
        memcpy(Buffer, pResponse->Data, pResponse->BytesRead);
    }
    
    VirtualFree(pBuffer, 0, MEM_RELEASE);
    return true;
}

bool DriverHandle::WriteMemory(ULONG64 Address, SIZE_T Size, LPCVOID Buffer) {
    if (!IsOpen() || !Buffer || Size == 0) {
        ERROR_LOG("Invalid parameters for WriteMemory");
        return false;
    }
    
    // Allocate buffer for request
    SIZE_T RequestSize = sizeof(WRITE_MEMORY_REQUEST) + Size;
    PWRITE_MEMORY_REQUEST pRequest = (PWRITE_MEMORY_REQUEST)VirtualAlloc(NULL, RequestSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!pRequest) {
        ERROR_LOG("Failed to allocate memory for WriteMemory");
        return false;
    }
    
    pRequest->Address = Address;
    pRequest->Size = Size;
    memcpy(pRequest->Data, Buffer, Size);
    
    DWORD BytesReturned = 0;
    bool Result = DeviceIoControl(
        m_hDevice,
        IOCTL_WRITE_MEMORY,
        pRequest,
        RequestSize,
        NULL,
        0,
        &BytesReturned,
        NULL
    );
    
    VirtualFree(pRequest, 0, MEM_RELEASE);
    
    if (!Result) {
        m_dwLastError = GetLastError();
        ERROR_LOG("WriteMemory DeviceIoControl failed: %lu", m_dwLastError);
        return false;
    }
    
    DEBUG_LOG("WriteMemory: Success");
    return true;
}

bool DriverHandle::GetProcessBaseAddress(ULONG ProcessId, ULONG64* BaseAddress) {
    if (!IsOpen() || !BaseAddress) {
        ERROR_LOG("Invalid parameters for GetProcessBaseAddress");
        return false;
    }
    
    GET_PROCESS_BASE_REQUEST Request = { 0 };
    ADDRESS_RESPONSE Response = { 0 };
    DWORD BytesReturned = 0;
    
    Request.ProcessId = ProcessId;
    
    bool Result = DeviceIoControl(
        m_hDevice,
        IOCTL_GET_PROCESS_BASE,
        &Request,
        sizeof(Request),
        &Response,
        sizeof(Response),
        &BytesReturned,
        NULL
    );
    
    if (!Result) {
        m_dwLastError = GetLastError();
        ERROR_LOG("GetProcessBaseAddress DeviceIoControl failed: %lu", m_dwLastError);
        return false;
    }
    
    if (!NT_SUCCESS(Response.Status)) {
        ERROR_LOG("GetProcessBaseAddress: Driver returned error: 0x%X", Response.Status);
        return false;
    }
    
    *BaseAddress = Response.Address;
    return true;
}

bool DriverHandle::GetModuleBaseAddress(ULONG ProcessId, LPCSTR ModuleName, ULONG64* BaseAddress) {
    if (!IsOpen() || !ModuleName || !BaseAddress) {
        ERROR_LOG("Invalid parameters for GetModuleBaseAddress");
        return false;
    }
    
    GET_MODULE_BASE_REQUEST Request = { 0 };
    ADDRESS_RESPONSE Response = { 0 };
    DWORD BytesReturned = 0;
    
    Request.ProcessId = ProcessId;
    strncpy_s(Request.ModuleName, ModuleName, sizeof(Request.ModuleName) - 1);
    
    bool Result = DeviceIoControl(
        m_hDevice,
        IOCTL_GET_MODULE_BASE,
        &Request,
        sizeof(Request),
        &Response,
        sizeof(Response),
        &BytesReturned,
        NULL
    );
    
    if (!Result) {
        m_dwLastError = GetLastError();
        ERROR_LOG("GetModuleBaseAddress DeviceIoControl failed: %lu", m_dwLastError);
        return false;
    }
    
    if (!NT_SUCCESS(Response.Status)) {
        ERROR_LOG("GetModuleBaseAddress: Driver returned error: 0x%X", Response.Status);
        return false;
    }
    
    *BaseAddress = Response.Address;
    return true;
}

// ============================================
// RobloxProcess Implementation
// ============================================

RobloxProcess::RobloxProcess() : m_dwProcessId(0), m_hProcess(NULL), m_ullBaseAddress(0) {}

RobloxProcess::~RobloxProcess() {
    Detach();
}

bool RobloxProcess::Attach(LPCWSTR ProcessName) {
    Detach();
    
    DEBUG_LOG("Attaching to process: %ws", ProcessName);
    
    // Find process ID
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        ERROR_LOG("CreateToolhelp32Snapshot failed: %lu", GetLastError());
        return false;
    }
    
    PROCESSENTRY32W pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    bool Found = false;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, ProcessName) == 0) {
                m_dwProcessId = pe32.th32ProcessID;
                Found = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    
    if (!Found) {
        ERROR_LOG("Process not found: %ws", ProcessName);
        return false;
    }
    
    DEBUG_LOG("Found process ID: %lu", m_dwProcessId);
    
    // Open process
    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_dwProcessId);
    if (!m_hProcess) {
        ERROR_LOG("OpenProcess failed: %lu", GetLastError());
        return false;
    }
    
    // Get base address via driver
    if (!m_Driver.Open()) {
        ERROR_LOG("Failed to open driver");
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
        return false;
    }
    
    if (!m_Driver.GetProcessBaseAddress(m_dwProcessId, &m_ullBaseAddress)) {
        ERROR_LOG("Failed to get process base address");
        m_Driver.Close();
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
        return false;
    }
    
    DEBUG_LOG("Process attached successfully, BaseAddress: 0x%llX", m_ullBaseAddress);
    return true;
}

void RobloxProcess::Detach() {
    if (m_hProcess) {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }
    m_dwProcessId = 0;
    m_ullBaseAddress = 0;
    m_Driver.Close();
    
    DEBUG_LOG("Process detached");
}

bool RobloxProcess::IsAttached() const {
    return m_hProcess != NULL && m_dwProcessId != 0;
}

DWORD RobloxProcess::GetProcessId() const {
    return m_dwProcessId;
}

HANDLE RobloxProcess::GetProcessHandle() const {
    return m_hProcess;
}

ULONG64 RobloxProcess::GetBaseAddress() const {
    return m_ullBaseAddress;
}

ULONG64 RobloxProcess::GetModuleBaseAddress(LPCSTR ModuleName) {
    if (!IsAttached()) {
        ERROR_LOG("Process not attached");
        return 0;
    }
    
    ULONG64 BaseAddress = 0;
    if (m_Driver.GetModuleBaseAddress(m_dwProcessId, ModuleName, &BaseAddress)) {
        DEBUG_LOG("Module %s base address: 0x%llX", ModuleName, BaseAddress);
        return BaseAddress;
    }
    
    ERROR_LOG("Failed to get module base address: %s", ModuleName);
    return 0;
}

bool RobloxProcess::ReadMemory(ULONG64 Address, SIZE_T Size, LPVOID Buffer) {
    if (!IsAttached()) {
        ERROR_LOG("Process not attached");
        return false;
    }
    
    return m_Driver.ReadMemory(Address, Size, Buffer);
}

bool RobloxProcess::WriteMemory(ULONG64 Address, SIZE_T Size, LPCVOID Buffer) {
    if (!IsAttached()) {
        ERROR_LOG("Process not attached");
        return false;
    }
    
    return m_Driver.WriteMemory(Address, Size, Buffer);
}

// ============================================
// RobloxMemory Implementation
// ============================================

bool RobloxMemory::FindPattern(ULONG64 StartAddress, SIZE_T Size, const char* Pattern, const char* Mask, ULONG64* FoundAddress) {
    if (!Pattern || !Mask || !FoundAddress) {
        return false;
    }
    
    SIZE_T PatternLength = strlen(Mask);
    if (PatternLength == 0 || PatternLength > Size) {
        return false;
    }
    
    for (ULONG64 i = 0; i <= Size - PatternLength; i++) {
        bool Match = true;
        for (SIZE_T j = 0; j < PatternLength; j++) {
            if (Mask[j] != '?' && Pattern[j] != *(char*)(StartAddress + i + j)) {
                Match = false;
                break;
            }
        }
        if (Match) {
            *FoundAddress = StartAddress + i;
            return true;
        }
    }
    
    return false;
}

bool RobloxMemory::FindSignature(const RobloxProcess& Process, const char* ModuleName, const char* Pattern, const char* Mask, ULONG64* FoundAddress) {
    if (!Process.IsAttached() || !ModuleName || !Pattern || !Mask || !FoundAddress) {
        return false;
    }
    
    ULONG64 ModuleBase = Process.GetModuleBaseAddress(ModuleName);
    if (ModuleBase == 0) {
        return false;
    }
    
    // Get module size
    MODULEINFO ModuleInfo = { 0 };
    if (!GetModuleInformation(Process.GetProcessHandle(), (HMODULE)ModuleBase, &ModuleInfo, sizeof(ModuleInfo))) {
        ERROR_LOG("GetModuleInformation failed: %lu", GetLastError());
        return false;
    }
    
    // Allocate buffer for module memory
    SIZE_T ModuleSize = ModuleInfo.SizeOfImage;
    PVOID pBuffer = VirtualAlloc(NULL, ModuleSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pBuffer) {
        ERROR_LOG("Failed to allocate buffer for module memory");
        return false;
    }
    
    // Read module memory
    SIZE_T BytesRead = 0;
    if (!ReadProcessMemory(Process.GetProcessHandle(), (LPCVOID)ModuleBase, pBuffer, ModuleSize, &BytesRead)) {
        ERROR_LOG("ReadProcessMemory failed: %lu", GetLastError());
        VirtualFree(pBuffer, 0, MEM_RELEASE);
        return false;
    }
    
    // Find pattern
    bool Found = FindPattern((ULONG64)pBuffer, BytesRead, Pattern, Mask, FoundAddress);
    
    VirtualFree(pBuffer, 0, MEM_RELEASE);
    
    if (Found) {
        *FoundAddress += ModuleBase;
    }
    
    return Found;
}

ULONG64 RobloxMemory::ResolvePointerChain(const RobloxProcess& Process, ULONG64 BaseAddress, const std::vector<ULONG64>& Offsets) {
    if (!Process.IsAttached() || Offsets.empty()) {
        return 0;
    }
    
    ULONG64 CurrentAddress = BaseAddress;
    
    for (size_t i = 0; i < Offsets.size(); i++) {
        ULONG64 PointerValue = 0;
        if (!Process.ReadMemory(CurrentAddress + Offsets[i], sizeof(ULONG64), &PointerValue)) {
            ERROR_LOG("Failed to read pointer at offset %llX", Offsets[i]);
            return 0;
        }
        
        CurrentAddress = PointerValue;
    }
    
    return CurrentAddress;
}

bool RobloxMemory::FindDataModel(const RobloxProcess& Process, ULONG64* DataModelAddress) {
    // Roblox-specific pattern for DataModel
    // This is a placeholder - actual patterns need to be updated for Roblox version 9affbe66b2624d20
    const char* Pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x40\x00";
    const char* Mask = "xxx????xxxx?xxx?";
    
    return FindSignature(Process, "RobloxPlayerBeta.exe", Pattern, Mask, DataModelAddress);
}

bool RobloxMemory::FindGameObject(const RobloxProcess& Process, ULONG64* GameObjectAddress) {
    // Placeholder for Game object pattern
    const char* Pattern = "\x48\x8D\x05\x00\x00\x00\x00\x48\x8B\xD9\xE8\x00\x00\x00\x00";
    const char* Mask = "xxx????xxxx????";
    
    return FindSignature(Process, "RobloxPlayerBeta.exe", Pattern, Mask, GameObjectAddress);
}

// ============================================
// LuaExecutor Implementation
// ============================================

LuaExecutor::LuaExecutor() : m_pProcess(NULL), m_ullLuaState(0), m_ullLuaLoad(0), m_ullLuaPCall(0) {}

LuaExecutor::~LuaExecutor() {
    m_pProcess = NULL;
    m_ullLuaState = 0;
    m_ullLuaLoad = 0;
    m_ullLuaPCall = 0;
}

bool LuaExecutor::Initialize(const RobloxProcess& Process) {
    if (!Process.IsAttached()) {
        ERROR_LOG("Process not attached");
        return false;
    }
    
    m_pProcess = const_cast<RobloxProcess*>(&Process);
    
    // Find Lua functions in Roblox memory
    // These offsets need to be updated for Roblox version 9affbe66b2624d20
    
    // Find Lua state
    ULONG64 DataModelAddress = 0;
    if (!RobloxMemory::FindDataModel(Process, &DataModelAddress)) {
        ERROR_LOG("Failed to find DataModel");
        return false;
    }
    
    DEBUG_LOG("DataModel found at: 0x%llX", DataModelAddress);
    
    // Find Lua functions (placeholder - actual implementation depends on Roblox version)
    // m_ullLuaState = ...;
    // m_ullLuaLoad = ...;
    // m_ullLuaPCall = ...;
    
    DEBUG_LOG("LuaExecutor initialized");
    return true;
}

bool LuaExecutor::ExecuteScript(const char* Script) {
    if (!m_pProcess || !m_pProcess->IsAttached() || !Script) {
        ERROR_LOG("Invalid state for ExecuteScript");
        return false;
    }
    
    DEBUG_LOG("Executing Lua script: %s", Script);
    
    // TODO: Implement actual Lua execution
    // This requires knowledge of Roblox's Lua VM structure
    
    ERROR_LOG("Lua execution not yet implemented");
    return false;
}

bool LuaExecutor::ExecuteBytecode(const unsigned char* Bytecode, SIZE_T Size) {
    if (!m_pProcess || !m_pProcess->IsAttached() || !Bytecode || Size == 0) {
        ERROR_LOG("Invalid state for ExecuteBytecode");
        return false;
    }
    
    DEBUG_LOG("Executing Lua bytecode, Size: %zu", Size);
    
    // TODO: Implement actual bytecode execution
    // This requires knowledge of Roblox's Lua VM structure
    
    ERROR_LOG("Bytecode execution not yet implemented");
    return false;
}

// ============================================
// Global Variables
// ============================================

static RobloxProcess g_RobloxProcess;
static LuaExecutor g_LuaExecutor;
static bool g_bInitialized = false;

// ============================================
// Export Functions Implementation
// ============================================

extern "C" {
    ROBLOX_API bool __cdecl InitializeExecutor() {
        if (g_bInitialized) {
            DEBUG_LOG("Executor already initialized");
            return true;
        }
        
        DEBUG_LOG("Initializing Executor...");
        
        // Attach to Roblox process
        if (!g_RobloxProcess.Attach()) {
            ERROR_LOG("Failed to attach to Roblox process");
            return false;
        }
        
        // Initialize Lua executor
        if (!g_LuaExecutor.Initialize(g_RobloxProcess)) {
            ERROR_LOG("Failed to initialize Lua executor");
            g_RobloxProcess.Detach();
            return false;
        }
        
        g_bInitialized = true;
        DEBUG_LOG("Executor initialized successfully");
        return true;
    }
    
    ROBLOX_API bool __cdecl ExecuteLuaScript(const char* Script) {
        if (!g_bInitialized) {
            ERROR_LOG("Executor not initialized");
            return false;
        }
        
        return g_LuaExecutor.ExecuteScript(Script);
    }
    
    ROBLOX_API bool __cdecl ExecuteLuaBytecode(const unsigned char* Bytecode, SIZE_T Size) {
        if (!g_bInitialized) {
            ERROR_LOG("Executor not initialized");
            return false;
        }
        
        return g_LuaExecutor.ExecuteBytecode(Bytecode, Size);
    }
    
    ROBLOX_API void __cdecl ShutdownExecutor() {
        if (!g_bInitialized) {
            return;
        }
        
        DEBUG_LOG("Shutting down Executor...");
        g_LuaExecutor.~LuaExecutor();
        new (&g_LuaExecutor) LuaExecutor();
        g_RobloxProcess.Detach();
        g_bInitialized = false;
        
        DEBUG_LOG("Executor shut down");
    }
    
    ROBLOX_API bool __cdecl IsDriverLoaded() {
        DriverHandle Driver;
        return Driver.Open();
    }
    
    ROBLOX_API bool __cdecl TestDriverConnection() {
        DriverHandle Driver;
        if (!Driver.Open()) {
            return false;
        }
        return Driver.TestConnection();
    }
}
