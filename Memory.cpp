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
uint64_t heap_base = 0x9000000;
uint64_t heap_size = 0x2000000;
//uint64_t heap_size = 0x0;
uint8_t *pRAM;	// XXX: Replace raw pointer to View wrapper.
static RAMBlock mem_map[] =
{
	RAMBlock (".text", 0x0, 0x1000000, PROT_READ | PROT_WRITE | PROT_EXEC),
	RAMBlock (".rdata", 0x1000000, 0x1000000, PROT_READ | PROT_WRITE),
	RAMBlock (".data", 0x2000000, 0x1000000, PROT_READ | PROT_WRITE),
	RAMBlock ("[stack]", 0x3000000, 0x6000000, PROT_READ | PROT_WRITE),
	RAMBlock ("[heap]", heap_base, heap_size, PROT_READ | PROT_WRITE),
};

void InitMemmap(Nsemu *nsemu) {
        void *data;
	if ((data = mmap (nullptr, 0x10000000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
	 	ns_abort ("Failed to allocate host memory\n");
	}
        pRAM = (uint8_t *) data;
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

std::list<std::tuple<uint64_t,uint64_t, int>> GetRegions() {
        std::list<std::tuple<uint64_t,uint64_t, int>> ret;
        uint64_t last;
        for (int i = 0; i < sizeof(mem_map) / sizeof(RAMBlock); i++) {
                uint64_t addr = mem_map[i].addr;
                size_t length = mem_map[i].length;
                int perm = mem_map[i].perm;
                ret.push_back(make_tuple(addr, addr + length, perm));
                last = addr + length + 1;
        }
        ret.push_back(make_tuple(last, 0xFFFFFFFFFFFFFFFF, -1));
        return ret;
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

bool CopyfromEmu(Nsemu *nsemu, void *data, uint64_t gpa, size_t len) {
	return _CopyMemEmu (data, gpa, len, false);
}

}
