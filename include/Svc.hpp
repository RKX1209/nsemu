#ifndef _SVC_HPP
#define _SVC_HPP

namespace SVC {

extern std::function<void()> svc_handlers[0x80];

void Init();

enum GetInfoType {
    // 1.0.0+
    AllowedCpuIdBitmask = 0,
    AllowedThreadPrioBitmask = 1,
    MapRegionBaseAddr = 2,
    MapRegionSize = 3,
    HeapRegionBaseAddr = 4,
    HeapRegionSize = 5,
    TotalMemoryUsage = 6,
    TotalHeapUsage = 7,
    IsCurrentProcessBeingDebugged = 8,
    ResourceHandleLimit = 9,
    IdleTickCount = 10,
    RandomEntropy = 11,
    PerformanceCounter = 0xF0000002,
    // 2.0.0+
    AddressSpaceBaseAddr = 12,
    AddressSpaceSize = 13,
    NewMapRegionBaseAddr = 14,
    NewMapRegionSize = 15,
    // 3.0.0+
    IsVirtualAddressMemoryEnabled = 16,
    PersonalMmHeapUsage = 17,
    TitleId = 18,
    // 4.0.0+
    PrivilegedProcessId = 19,
    // 5.0.0+
    UserExceptionContextAddr = 20,
};

std::tuple<uint64_t, uint64_t> SetHeapSize(uint64_t size);
uint64_t SetMemoryAttribute(uint64_t addr, uint64_t size, uint64_t state0, uint64_t state1);
uint64_t MirrorStack(uint64_t dest, uint64_t src, uint64_t size);
uint64_t UnmapMemory(uint64_t dest, uint64_t src, uint64_t size);
std::tuple<uint64_t, uint64_t> QueryMemory(uint64_t meminfo, uint64_t pageinfo, uint64_t addr);
void ExitProcess(uint64_t exitCode);
std::tuple<uint64_t, uint64_t> CreateThread(uint64_t pc, uint64_t x0, uint64_t sp, uint64_t prio, uint64_t proc);
uint64_t StartThread(uint32_t handle);
void ExitThread();
uint64_t SleepThread(uint64_t ns);
std::tuple<uint64_t, uint64_t> GetThreadPriority(uint32_t handle);
uint64_t SetThreadPriority(uint32_t handle, uint64_t priority);
std::tuple<uint64_t, uint64_t, uint64_t> GetThreadCoreMask(uint64_t tmp);
uint64_t SetThreadCoreMask(uint64_t tmp);
uint64_t GetCurrentProcessorNumber(uint64_t tmp);
uint64_t SignalEvent(uint32_t handle);
uint64_t ClearEvent(uint32_t handle);
uint64_t MapMemoryBlock(uint32_t handle, uint64_t addr, uint64_t size, uint64_t perm);
std::tuple<uint64_t, uint64_t> CreateTransferMemory(uint64_t addr, uint64_t size, uint64_t perm);
uint64_t CloseHandle(uint32_t handle);
uint64_t ResetSignal(uint32_t handle);
std::tuple<uint64_t, uint64_t> WaitSynchronization(uint64_t handles, uint64_t numHandles, uint64_t timeout);
uint64_t CancelSynchronization(uint32_t handle);
uint64_t ensureMutex(uint64_t mutexAddr);
uint64_t LockMutex(uint32_t curthread, uint64_t mutexAddr, uint32_t reqthread);
void UnlockMutex(uint64_t mutexAddr);
uint64_t ensureSemaphore(uint64_t semaAddr);
void WaitProcessWideKeyAtomic(uint64_t mutexAddr, uint64_t semaAddr, uint32_t threadHandle, uint64_t timeout);
uint64_t SignalProcessWideKey(uint64_t semaAddr, uint64_t target);
std::tuple<uint64_t, uint32_t> ConnectToPort(uint64_t name);
uint64_t SendSyncRequest(uint32_t handle);
uint64_t SendSyncRequestEx(uint64_t buf, uint64_t size, uint32_t handle);
std::tuple<uint64_t, uint64_t> GetProcessID(uint32_t handle);
std::tuple<uint64_t, uint64_t> GetThreadId();
uint64_t Break(uint64_t X0, uint64_t X1, uint64_t info);
uint64_t OutputDebugString(uint64_t ptr, uint64_t size);
std::tuple<uint64_t, uint64_t> GetInfo(uint64_t id1, uint32_t handle, uint64_t id2);
std::tuple<uint64_t, uint64_t, uint64_t> CreateSession(uint32_t clientOut, uint32_t serverOut, uint64_t unk);
std::tuple<uint64_t, uint64_t> AcceptSession(uint32_t hnd);
std::tuple<uint64_t, uint64_t> ReplyAndReceive(uint64_t handles, uint64_t numHandles, uint32_t replySession, uint64_t timeout);
std::tuple<uint64_t, uint64_t, uint64_t> CreateEvent(uint32_t clientOut, uint32_t serverOut, uint64_t unk);
std::tuple<uint64_t, uint64_t> ReadWriteRegister(uint64_t reg, uint64_t rwm, uint64_t val);
std::tuple<uint64_t, uint64_t> CreateMemoryBlock(uint64_t size, uint64_t perm);
uint64_t MapTransferMemory(uint32_t handle, uint64_t addr, uint64_t size, uint64_t perm);
uint64_t UnmapTransferMemory(uint32_t handle, uint64_t addr, uint64_t size);
std::tuple<uint64_t, uint64_t> CreateInterruptEvent(uint64_t irq);
std::tuple<uint64_t, uint64_t> QueryIoMapping(uint64_t physaddr, uint64_t size);
std::tuple<uint64_t, uint64_t> CreateDeviceAddressSpace(uint64_t base, uint64_t size);
std::tuple<uint64_t, uint64_t> AttachDeviceAddressSpace(uint32_t handle, uint64_t dev, uint64_t addr);
std::tuple<uint64_t, uint64_t> MapDeviceAddressSpaceByForce(uint32_t handle, uint32_t phandle, uint64_t paddr, uint64_t size, uint64_t maddr, uint64_t perm);
uint64_t UnmapDeviceAddressSpace(uint64_t unk0, uint32_t phandle, uint64_t maddr, uint64_t size);
uint64_t MapProcessMemory(uint64_t dstaddr, uint32_t handle, uint64_t srcaddr, uint64_t size);
uint64_t UnmapProcessMemory(uint64_t dstaddr, uint32_t handle, uint64_t srcaddr, uint64_t size);
uint64_t MapProcessCodeMemory(uint32_t handle, uint64_t dstaddr, uint64_t srcaddr, uint64_t size);
uint64_t UnmapProcessCodeMemory(uint32_t handle, uint64_t dstaddr, uint64_t srcaddr, uint64_t size);

};

#endif
