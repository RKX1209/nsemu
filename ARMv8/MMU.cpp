/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

namespace ARMv8 {

uint32_t ReadInst(uint64_t vaddr) {
	/* XXX: Implement Page translation */
	const uint64_t paddr = vaddr;
	return ReadU32 (paddr);
}

template<typename T>
static T ReadFromRAM(const uint64_t paddr) {
	T value = 0;
	for (uint64_t addr = paddr; addr < paddr + sizeof(T); addr++) {
		uint8_t byte;
		std::memcpy (&byte, &Memory::pRAM[addr], sizeof(uint8_t));
		value = (value << 8) | byte;
	}
	return value;
}

uint32_t ReadU32(const uint64_t paddr) {
	return ReadFromRAM<uint32_t>(paddr);
}

}
