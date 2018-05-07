/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

#define RegisterSvc(num, func, ...) do { \
	RegisterSvcHandler((num), [&] { \
		(func)(__VA_ARGS__); \
	}); \
} while(0)

#define RegisterSvcRetX0(num, func, ...) do { \
	RegisterSvcHandler((num), [&] { \
		auto x0 = (func)(__VA_ARGS__); \
                X(0) = x0; \
	}); \
} while(0)

#define RegisterSvcRetX1(num, func, ...) do { \
	RegisterSvcHandler((num), [&] { \
		auto x1 = (func)(__VA_ARGS__); \
                X(1) = x1; \
	}); \
} while(0)

#define RegisterSvcRetX01(num, func, ...) do { \
	RegisterSvcHandler((num), [&] { \
		auto [x0, x1] = (func)(__VA_ARGS__); \
                X(0) = x0; \
                X(1) = x1; \
	}); \
} while(0)

#define RegisterSvcRetX012(num, func, ...) do { \
	RegisterSvcHandler((num), [&] { \
		auto [x0, x1, x2] = (func)(__VA_ARGS__); \
                X(0) = x0; \
                X(1) = x1; \
                X(2) = x2; \
	}); \
} while(0)


namespace SVC {

std::function<void()> svc_handlers[0x80];

void RegisterSvcHandler (unsigned int num, std::function<void()> handler) {
        svc_handlers[num] = handler;
}

void Init() {
        RegisterSvcRetX01(0x01, SetHeapSize, X(1));
        RegisterSvcRetX0(0x03, SetMemoryAttribute, X(0), X(1), X(2), X(3));
        RegisterSvcRetX0(0x04, MirrorStack, X(0), X(1), X(2));
        RegisterSvcRetX0(0x05, UnmapMemory, X(0), X(1), X(2));
        RegisterSvcRetX01(0x06, QueryMemory, X(0), X(1), X(2));
        RegisterSvc(0x07, ExitProcess, X(0));
        RegisterSvcRetX01(0x08, CreateThread, X(1), X(2), X(3), X(4), X(5));
        RegisterSvcRetX0(0x09, StartThread, (uint32_t) X(0));
        RegisterSvc(0x0A, ExitThread);
        RegisterSvcRetX0(0x0B, SleepThread, X(0));
        RegisterSvcRetX01(0x0C, GetThreadPriority, (uint32_t) X(0));
        RegisterSvcRetX0(0x0D, SetThreadPriority, (uint32_t) X(0), X(1));
        RegisterSvcRetX012(0x0E, GetThreadCoreMask, X(0));
        RegisterSvcRetX0(0x0F, SetThreadCoreMask, X(0));
        RegisterSvcRetX0(0x10, GetCurrentProcessorNumber, X(0));
        RegisterSvcRetX0(0x11, SignalEvent, (uint32_t) X(0));
        RegisterSvcRetX0(0x12, ClearEvent, (uint32_t) X(0));
        RegisterSvcRetX0(0x13, MapMemoryBlock, (uint32_t) X(0), X(1), X(2), X(3));
        RegisterSvcRetX01(0x15, CreateTransferMemory, X(0), X(1), X(2));
        RegisterSvcRetX0(0x16, CloseHandle, (uint32_t) X(0));
        RegisterSvcRetX0(0x17, ResetSignal, (uint32_t) X(0));
        RegisterSvcRetX01(0x18, WaitSynchronization, X(1), X(2), X(3));
        RegisterSvcRetX0(0x19, CancelSynchronization, (uint32_t) X(0));
        RegisterSvcRetX0(0x1A, LockMutex, (uint32_t) X(0), X(1), (uint32_t) X(2));
        RegisterSvc(0x1B, UnlockMutex, X(0));
        RegisterSvc(0x1C, WaitProcessWideKeyAtomic, X(0), X(1), (uint32_t) X(2), X(3));
        RegisterSvcRetX0(0x1D, SignalProcessWideKey, X(0), X(1));
        RegisterSvcRetX01(0x1F, ConnectToPort, X(1));
        RegisterSvcRetX0(0x21, SendSyncRequest, (uint32_t) X(0));
        RegisterSvcRetX0(0x22, SendSyncRequestEx, X(0), X(1), (uint32_t) X(2));
        RegisterSvcRetX01(0x24, GetProcessID, (uint32_t) X(1));
        RegisterSvcRetX01(0x25, GetThreadId);
        RegisterSvcRetX0(0x26, Break, X(0), X(1), X(2));
        RegisterSvcRetX0(0x27, OutputDebugString, X(0), X(1));
        RegisterSvcRetX01(0x29, GetInfo, X(1), (uint32_t) X(2), X(3));
        RegisterSvcRetX012(0x40, CreateSession, (uint32_t) X(0), (uint32_t) X(1), X(2));
        RegisterSvcRetX01(0x41, AcceptSession, (uint32_t) X(1));
        RegisterSvcRetX01(0x43, ReplyAndReceive, X(1), X(2), (uint32_t) X(3), X(4));
        RegisterSvcRetX012(0x45, CreateEvent, (uint32_t) X(0), (uint32_t) X(1), X(2));
        RegisterSvcRetX01(0x4E, ReadWriteRegister, X(1), X(2), X(3));
        RegisterSvcRetX01(0x50, CreateMemoryBlock, X(1), X(2));
        RegisterSvcRetX0(0x51, MapTransferMemory, (uint32_t) X(0), X(1), X(2), X(3));
        RegisterSvcRetX0(0x52, UnmapTransferMemory, (uint32_t) X(0), X(1), X(2));
        RegisterSvcRetX01(0x53, CreateInterruptEvent, X(1));
        RegisterSvcRetX01(0x55, QueryIoMapping, X(1), X(2));
        RegisterSvcRetX01(0x56, CreateDeviceAddressSpace, X(1), X(2));
        RegisterSvcRetX01(0x57, AttachDeviceAddressSpace, (uint32_t) X(0), X(1), X(2));
        RegisterSvcRetX01(0x59, MapDeviceAddressSpaceByForce, (uint32_t) X(0), (uint32_t) X(1), X(2), X(3), X(4), X(5));
        RegisterSvcRetX0(0x5c, UnmapDeviceAddressSpace, X(0), (uint32_t) X(1), X(2), X(3));
        RegisterSvcRetX0(0x74, MapProcessMemory, X(0), (uint32_t) X(1), X(2), X(3));
        RegisterSvcRetX0(0x75, UnmapProcessMemory, X(0), (uint32_t) X(1), X(2), X(3));
        RegisterSvcRetX0(0x77, MapProcessCodeMemory, (uint32_t) X(0), X(1), X(2), X(3));
        RegisterSvcRetX0(0x78, UnmapProcessCodeMemory, (uint32_t) X(0), X(1), X(2), X(3));

}
std::tuple<uint64_t, uint64_t> SetHeapSize(uint64_t size) {
	ns_print("SetHeapSize 0x%lx\n", size);
        if (Memory::heap_size < size) {
                Memory::AddMemmap (Memory::heap_base, size - Memory::heap_size);
        } else if (Memory::heap_size > size) {
                /* TODO: */
        }

        Memory::heap_size = size;
	return make_tuple(0, Memory::heap_base);
}

uint64_t SetMemoryAttribute(uint64_t addr, uint64_t size, uint64_t state0, uint64_t state1) {
	return 0;
}

uint64_t MirrorStack(uint64_t dest, uint64_t src, uint64_t size) {
        ns_print("MirrorStack 0x%lx 0x%lx 0x%lx\n", dest, src, size);
        Memory::AddMemmap (dest, size);
        uint8_t *temp = new uint8_t[size];
        ARMv8::ReadBytes(src, temp, size);
        ARMv8::WriteBytes(dest, temp, size);
        delete[] temp;
	return 0;
}

uint64_t UnmapMemory(uint64_t dest, uint64_t src, uint64_t size) {
        ns_print("UnmapMemory 0x%lx 0x%lx 0x%lx\n", dest, src, size);
        Memory::DelMemmap(dest, size);
	return 0;
}

typedef struct {
	uint64_t begin;
	uint64_t size;
	uint32_t memory_type;
	uint32_t memory_attribute;
	uint32_t permission;
	uint32_t device_ref_count;
	uint32_t ipc_ref_count;
	uint32_t padding;
} MemInfo;

std::tuple<uint64_t, uint64_t> QueryMemory(uint64_t meminfo, uint64_t pageinfo, uint64_t addr) {
        ns_print("QueryMemory 0x%lx\n", addr);
        for(auto [begin, end, perm] : Memory::GetRegions()) {
                if (begin <= addr && addr <= end) {
                        ns_print("found region at 0x%lx, 0x%lx\n", begin, end);
                        MemInfo minfo;
                        minfo.begin = begin;
                        minfo.size = end - begin + 1;
			minfo.memory_type = perm == -1 ? 0 : 3; // FREE or CODE
			minfo.memory_attribute = 0;
                        if(addr >= Memory::heap_base && addr < Memory::heap_base + Memory::heap_size) {
				minfo.memory_type = 5; // HEAP
			}
                        minfo.permission = 0;
			if(perm != -1) {
				auto offset = *ARMv8::GuestPtr<uint32_t>(begin + 4);
				if(begin + offset + 4 < end && *ARMv8::GuestPtr<uint32_t>(begin + offset) == byte_swap32_str("MOD0"))
					minfo.permission = 5;
				else
					minfo.permission = 3;
			}
                        MemInfo *ptr = ARMv8::GuestPtr<MemInfo>(meminfo);
                        *ptr = minfo;
                        break;
                }
        }
	return make_tuple(0, 0);
}

// the nintendo svc probably doesn't take an exitCode,
// but this makes it easier to return values from
// libtransistor tests.
void ExitProcess(uint64_t exitCode) {
	ns_print("ExitProcess\n");
	exit((int) exitCode);
}

std::tuple<uint64_t, uint64_t> CreateThread(uint64_t pc, uint64_t x0, uint64_t sp, uint64_t prio, uint64_t proc) {
	return make_tuple(0, 0);
}

uint64_t StartThread(uint32_t handle) {
	return 0;
}

void ExitThread() {
}

uint64_t SleepThread(uint64_t ns) {
        ns_print("SleepThread 0x%lx [ns]\n", ns);
	return 0;
}

std::tuple<uint64_t, uint64_t> GetThreadPriority(uint32_t handle) {
	return make_tuple(0, 0);
}

uint64_t SetThreadPriority(uint32_t handle, uint64_t priority) {
	return 0;
}

std::tuple<uint64_t, uint64_t, uint64_t> GetThreadCoreMask(uint64_t tmp) {
	ns_print("GetThreadCoreMask\n");
	return make_tuple(0, 0xFF, 0xFF);
}

uint64_t SetThreadCoreMask(uint64_t tmp) {
	ns_print("GetThreadCoreMask\n");
	return 0;
}

uint64_t GetCurrentProcessorNumber(uint64_t tmp) {
	ns_print("GetCurrentProcessorNumber\n");
	return 0;
}

uint64_t SignalEvent(uint32_t handle) {
	ns_print("SignalEvent 0x%x\n", handle);
	return 0;
}

uint64_t ClearEvent(uint32_t handle) {
	ns_print("ClearEvent 0x%x\n", handle);
	return 0;
}

uint64_t MapMemoryBlock(uint32_t handle, uint64_t addr, uint64_t size, uint64_t perm) {
	return 0;
}

std::tuple<uint64_t, uint64_t> CreateTransferMemory(uint64_t addr, uint64_t size, uint64_t perm) {
    return make_tuple(0, NULL);
}

uint64_t CloseHandle(uint32_t handle) {
	ns_print("CloseHandle 0x%x\n", handle);
	return 0;
}

uint64_t ResetSignal(uint32_t handle) {
	ns_print("ResetSignal 0x%x", handle);
	return 0;
}

std::tuple<uint64_t, uint64_t> WaitSynchronization(uint64_t handles, uint64_t numHandles, uint64_t timeout) {
	return make_tuple(0, 0);
}

uint64_t CancelSynchronization(uint32_t handle) {
	ns_print("CancelSynchronization 0x%x\n", handle);
	return 0;
}

uint64_t ensureMutex(uint64_t mutexAddr) {
	return 0;
}

uint64_t LockMutex(uint32_t curthread, uint64_t mutexAddr, uint32_t reqthread) {
	return 0;
}

void UnlockMutex(uint64_t mutexAddr) {
}

uint64_t ensureSemaphore(uint64_t semaAddr) {
	return 0;
}

void WaitProcessWideKeyAtomic(uint64_t mutexAddr, uint64_t semaAddr, uint32_t threadHandle, uint64_t timeout) {
}

uint64_t SignalProcessWideKey(uint64_t semaAddr, uint64_t target) {
	return 0;
}

std::tuple<uint64_t, uint32_t> ConnectToPort(uint64_t name) {
        std::string s_name = ARMv8::ReadString(name);
        ns_print("ConnectToPort %s\n", s_name.c_str());
	return make_tuple(0, IPC::ConnectToPort(s_name));
}

uint64_t SendSyncRequest(uint32_t handle) {
	ns_print("SendSyncRequest\n");
	uint8_t msgbuf[0x100];
        ARMv8::ReadBytes (ARMv8::GetTls(), msgbuf, 0x100);
        auto handler = IPC::GetHandle<IpcService*>(handle);
        if (!handler) {
                ns_abort ("Cannnot find session handler\n");
        }
        IPC::ProcMessage(handler, msgbuf);
        return 0;
}

uint64_t SendSyncRequestEx(uint64_t buf, uint64_t size, uint32_t handle) {
	ns_print("SendSyncRequestEx not implemented\n");
	return 0xf601;
}

std::tuple<uint64_t, uint64_t> GetProcessID(uint32_t handle) {
	ns_print("GetProcessID 0x%x", handle);
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t> GetThreadId() {
	return make_tuple(0, 0);
}

uint64_t Break(uint64_t X0, uint64_t X1, uint64_t info) {
	ns_print("Break\n");
	exit(1);
}

uint64_t OutputDebugString(uint64_t ptr, uint64_t size) {
        unsigned char *str = new unsigned char[size + 1];
        ARMv8::ReadBytes (ptr, str, size);
        ns_print("[PRINT]: %s\n", str);
        delete[] str;
	return 0;
}
#define matchone(a, v) do { if(id1 == (a)) return make_tuple(0, (v)); } while(0)
#define matchpair(a, b, v) do { if(id1 == (a) && id2 == (b)) return make_tuple(0, (v)); } while(0)
std::tuple<uint64_t, uint64_t> GetInfo(uint64_t id1, uint32_t handle, uint64_t id2) {
        ns_print("GetInfo id1: %llu, id2: %llu, handle: %u\n", id1, id2, handle);
        matchpair(0, 0, 0xF);
	matchpair(1, 0, 0xFFFFFFFF00000000);
	matchpair(2, 0, 0xbb0000000); // map region
	matchpair(3, 0, 0x1000000000); // size
	matchpair(4, 0, Memory::heap_base); // heap region
	matchpair(5, 0, Memory::heap_size); // size
	matchpair(6, 0, 0x400000);
	matchpair(7, 0, 0x10000);
	matchpair(12, 0, 0x8000000);
	matchpair(13, 0, 0x7ff8000000);
	matchpair(14, 0, 0xbb0000000); // new map region
	matchpair(15, 0, 0x1000000000); // size
	matchpair(18, 0, 0x0100000000000036); // Title ID
	matchone(11, 0);

        ns_abort ("Unknown getinfo %llu, %llu\n", id1, id2);
}

std::tuple<uint64_t, uint64_t, uint64_t> CreateSession(uint32_t clientOut, uint32_t serverOut, uint64_t unk) {
	return make_tuple(0, 0, 0);
}

std::tuple<uint64_t, uint64_t> AcceptSession(uint32_t hnd) {
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t> ReplyAndReceive(uint64_t handles, uint64_t numHandles, uint32_t replySession, uint64_t timeout) {
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t, uint64_t> CreateEvent(uint32_t clientOut, uint32_t serverOut, uint64_t unk) {
	ns_print("CreateEvent\n");
	return make_tuple(0, 0, 0);
}

std::tuple<uint64_t, uint64_t> ReadWriteRegister(uint64_t reg, uint64_t rwm, uint64_t val) {
	ns_print("ReadWriteRegister\n");
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t> CreateMemoryBlock(uint64_t size, uint64_t perm) {
	return make_tuple(0, 0);
}

uint64_t MapTransferMemory(uint32_t handle, uint64_t addr, uint64_t size, uint64_t perm) {
	return 0;
}

uint64_t UnmapTransferMemory(uint32_t handle, uint64_t addr, uint64_t size) {
	return 0;
}

std::tuple<uint64_t, uint64_t> CreateInterruptEvent(uint64_t irq) {
	ns_print("CreateInterruptEvent\n");
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t> QueryIoMapping(uint64_t physaddr, uint64_t size) {
	return make_tuple(0x0, 0);
}

std::tuple<uint64_t, uint64_t> CreateDeviceAddressSpace(uint64_t base, uint64_t size) {
	ns_print("CreateDeviceAddressSpace\n");
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t> AttachDeviceAddressSpace(uint32_t handle, uint64_t dev, uint64_t addr) {
	ns_print("AttachDeviceAddressSpace\n");
	return make_tuple(0, 0);
}

std::tuple<uint64_t, uint64_t> MapDeviceAddressSpaceByForce(uint32_t handle, uint32_t phandle, uint64_t paddr, uint64_t size, uint64_t maddr, uint64_t perm) {
	ns_print("MapDeviceAddressSpaceByForce\n");
	return make_tuple(0, 0);
}

uint64_t UnmapDeviceAddressSpace(uint64_t unk0, uint32_t phandle, uint64_t maddr, uint64_t size) {
	ns_print("UnmapDeviceAddressSpace\n");
	return 0;
}

uint64_t MapProcessMemory(uint64_t dstaddr, uint32_t handle, uint64_t srcaddr, uint64_t size) {
	return 0;
}

uint64_t UnmapProcessMemory(uint64_t dstaddr, uint32_t handle, uint64_t srcaddr, uint64_t size) {
	return 0;
}

uint64_t MapProcessCodeMemory(uint32_t handle, uint64_t dstaddr, uint64_t srcaddr, uint64_t size) {
	return 0;
}

uint64_t UnmapProcessCodeMemory(uint32_t handle, uint64_t dstaddr, uint64_t srcaddr, uint64_t size) {
	return 0;
}


};
