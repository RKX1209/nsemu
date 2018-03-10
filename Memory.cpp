/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include <sys/mman.h>
#include "Nsemu.hpp"

RAMBlock::RAMBlock (std::string _name, uint64_t _addr, size_t _length, int _perm) {
	int page = getpagesize ();
	name = _name;
	length = _length;
	perm = _perm;
	if (addr & (page - 1)) {
		addr = addr & ~(page - 1);
	}
	addr = _addr;
}

namespace Memory
{
uint8_t *pRAM;	// XXX: Replace raw pointer to View wrapper.
static RAMBlock mem_map[] =
{
	RAMBlock (".text", 0x0, 0x1000000, PROT_READ | PROT_WRITE | PROT_EXEC),
	RAMBlock (".rdata", 0x1000000, 0x1000000, PROT_READ | PROT_WRITE),
	RAMBlock (".data", 0x2000000, 0x1000000, PROT_READ | PROT_WRITE),
	RAMBlock ("[stack]", 0x3000000, 0x6000000, PROT_READ | PROT_WRITE),
};

void InitMemmap(Nsemu *nsemu) {
        void *data;
	if ((data = mmap (nullptr, 0x10000000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
	 	ns_abort ("Failed to allocate host memory\n");
	}
        pRAM = (uint8_t *) data;
	int num = sizeof(mem_map) / sizeof(RAMBlock);
	for (int n = 0; n < num; n++) {
		std::string sec_name = mem_map[n].name;
		nsemu->rams[sec_name] = mem_map[n];
	}
}

RAMBlock *FindRAMBlock(Nsemu *nsemu, uint64_t addr, size_t len) {
	RAMBlock *as;
	std::map<std::string, RAMBlock>::iterator it = nsemu->rams.begin ();
	for (; it != nsemu->rams.end (); it++) {
		as = &it->second;
		if (as->addr <= addr && addr + len <= as->addr + as->length) {
			return as;
		}
	}
	return nullptr;
}

static bool _CopyMemEmu(void *data, uint64_t gpa, size_t len, bool load) {
	void *emu_mem = (void *)&pRAM[gpa];
	if (load) {
		memcpy (emu_mem, data, len);
	} else {
		memcpy (data, emu_mem, len);
	}
	return true;
}

bool CopytoEmu(Nsemu *nsemu, void *data, uint64_t gpa, size_t len) {
	return _CopyMemEmu (data, gpa, len, true);
}

bool CopytoEmuByName(Nsemu *nsemu, void *data, std::string name, size_t len) {
	if (nsemu->rams.find (name) == nsemu->rams.end ()) {
		return false;
	}
	RAMBlock *as = &nsemu->rams[name];
	if (len > as->length) {
		return false;
	}
	return _CopyMemEmu (data, as->addr, len, true);
}

bool CopyfromEmu(Nsemu *nsemu, void *data, uint64_t gpa, size_t len) {
	return _CopyMemEmu (data, gpa, len, false);
}

bool CopyfromEmuByName(Nsemu *nsemu, void *data, std::string name, size_t len) {
	if (nsemu->rams.find (name) == nsemu->rams.end ()) {
		return false;
	}
	RAMBlock *as = &nsemu->rams[name];
	if (len > as->length) {
		return false;
	}
	return _CopyMemEmu (data, as->addr, len, false);
}

}
