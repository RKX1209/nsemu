/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

namespace ARMv8 {

uint32_t ReadInst(uint64_t gva) {
	return ReadU32 (gva);
}

uint64_t GvaToHva(const uint64_t gva) {
        uint64_t gpa = gva;
        return (uint64_t) &Memory::pRAM[gpa];
}

template<typename T>
static T ReadFromRAM(const uint64_t gpa) {
	T value = 0;
        uint8_t *emu_mem = static_cast<uint8_t *>(Memory::GetRawPtr(gpa, sizeof(T)));
        debug_print("ReadFromRAM: 0x%lx, (%d)\n", gpa, sizeof(T));
	for (uint64_t addr = gpa; addr < gpa + sizeof(T); addr++) {
		uint8_t byte;
		std::memcpy (&byte, &emu_mem[addr - gpa], sizeof(uint8_t));
		value = value | ((uint64_t)byte << (8 * (addr - gpa)));
	}
        if (GdbStub::enabled) {
                GdbStub::NotifyMemAccess (gpa, sizeof(T), true);
        }
	return value;
}

template<typename T>
static void WriteToRAM(const uint64_t gpa, T value) {
        uint8_t *emu_mem = static_cast<uint8_t *>(Memory::GetRawPtr(gpa, sizeof(T)));
        debug_print("WriteToRAM: 0x%lx, (%d) RawPtr(%p)\n", gpa, sizeof(T), (void *)emu_mem);
	for (uint64_t addr = gpa; addr < gpa + sizeof(T); addr++) {
                uint8_t byte = value & 0xff;
		std::memcpy (&emu_mem[addr - gpa], &byte, sizeof(uint8_t));
		value >>= 8;
	}
        if (GdbStub::enabled) {
                GdbStub::NotifyMemAccess (gpa, sizeof(T), false);
        }
}

void ReadBytes(uint64_t gva, uint8_t *ptr, int size) {
        uint64_t gpa = gva;
        for (int i = 0; i < size; i++) {
                uint8_t byte = ReadU8 (gpa + i);
                ptr[i] = byte;
        }
}

void GdbReadBytes(uint64_t gva, uint8_t *ptr, int size) {
        /* XXX: temporaly, disable GdbStub */
        bool enabled = GdbStub::enabled;
        GdbStub::enabled = false;
        ReadBytes (gva, ptr, size);
        GdbStub::enabled = enabled;
}

std::string ReadString(uint64_t gva) {
        uint64_t gpa = gva;
        int mx_size = (1 << 30);
        char byte;
        int sz = 0;
        do {
                byte = (char)ReadU8(gpa + sz);
                sz++;
        } while (sz < mx_size && byte != '\0');
        if (sz >= mx_size) {
                ns_abort("Can not find any string from addr 0x%lx\n", gva);
        }
        char *bytes = new char[sz];
        for (int i = 0; i < sz; i++) {
                bytes[i] = (char)ReadU8(gpa + i);
        }
        std::string str = std::string(bytes);
        delete[] bytes;
        return str;
}
void WriteBytes(uint64_t gva, uint8_t *ptr, int size) {
        uint64_t gpa = gva;
        for (int i = 0; i < size; i++) {
                WriteU8 (gpa + i, ptr[i]);
        }
}

void GdbWriteBytes(uint64_t gva, uint8_t *ptr, int size) {
        /* XXX: temporaly, disable GdbStub */
        bool enabled = GdbStub::enabled;
        GdbStub::enabled = false;
        WriteBytes (gva, ptr, size);
        GdbStub::enabled = enabled;
}

uint8_t ReadU8(const uint64_t gva) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	return ReadFromRAM<uint8_t>(gpa);
}
uint16_t ReadU16(const uint64_t gva) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	return ReadFromRAM<uint16_t>(gpa);
}
uint32_t ReadU32(const uint64_t gva) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	return ReadFromRAM<uint32_t>(gpa);
}
uint64_t ReadU64(const uint64_t gva) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	return ReadFromRAM<uint64_t>(gpa);
}

void WriteU8(const uint64_t gva, uint8_t value) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	WriteToRAM<uint8_t>(gpa, value);
}
void WriteU16(const uint64_t gva, uint16_t value) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	WriteToRAM<uint16_t>(gpa, value);
}
void WriteU32(const uint64_t gva, uint32_t value) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	WriteToRAM<uint32_t>(gpa, value);
}
void WriteU64(const uint64_t gva, uint64_t value) {
	/* XXX: Implement Page translation */
	uint64_t gpa = gva;
	WriteToRAM<uint64_t>(gpa, value);
}

}
