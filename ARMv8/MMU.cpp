/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

namespace ARMv8 {

uint32_t ReadInst(uint64_t gva) {
	return ReadU32 (gva);
}

template<typename T>
static T ReadFromRAM(const uint64_t gpa) {
	T value = 0;
	for (uint64_t addr = gpa; addr < gpa + sizeof(T); addr++) {
		uint8_t byte;
		std::memcpy (&byte, &Memory::pRAM[addr], sizeof(uint8_t));
		value = (value << 8) | byte;
	}
	return value;
}

template<typename T>
static void WriteToRAM(const uint64_t gpa, T value) {
	std::memcpy (&Memory::pRAM[gpa], &value, sizeof(T));
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
