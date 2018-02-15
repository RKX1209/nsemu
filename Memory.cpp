/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include <sys/mman.h>
#include "Nsemu.hpp"

AddressSpace::AddressSpace (std::string _name, uint64_t addr, size_t _length, int _perm, uint8_t **out_pointer) {
	int page = getpagesize ();
	name = _name;
	length = _length;
	perm = _perm;
	if (addr & (page - 1)) {
		addr = addr & ~(page - 1);
	}
	if ((data = mmap ((void *) addr, length, perm, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		ns_abort ("Failed to mmap %s\n", name.c_str ());
	}
	debug_print ("mmap %s: %p\n", name.c_str (), data);
	addr = (uint64_t) data;
	if (out_pointer) {
		*out_pointer = (uint8_t *) data;
	}
}

namespace Memory
{
uint8_t *pRAM;	// XXX: Replace raw pointer to View wrapper.
static AddressSpace mem_map[] =
{
	AddressSpace (".text", (uint64_t) nullptr, 0x100000, PROT_READ | PROT_WRITE | PROT_EXEC, &pRAM),
	AddressSpace (".rdata", (uint64_t) nullptr, 0x100000, PROT_READ | PROT_WRITE, NULL),
	AddressSpace (".data", (uint64_t) nullptr, 0x100000, PROT_READ | PROT_WRITE, NULL),
};

void InitMemmap(Nsemu *nsemu) {
	int num = sizeof(mem_map) / sizeof(AddressSpace);
	for (int n = 0; n < num; n++) {
		std::string sec_name = mem_map[n].name;
		nsemu->as[sec_name] = mem_map[n];
	}
}

AddressSpace *FindAddressSpace(Nsemu *nsemu, uint64_t addr, size_t len) {
	AddressSpace *as;
	std::map<std::string, AddressSpace>::iterator it = nsemu->as.begin ();
	for (; it != nsemu->as.end (); it++) {
		as = &it->second;
		if (as->addr <= addr && addr + len <= as->addr + as->length) {
			return as;
		}
	}
	return nullptr;
}

static bool _CopyMemEmu(AddressSpace *as, void *data, uint64_t addr, size_t len, bool load) {
	uint64_t off = addr - as->addr;
	void *emu_mem = (uint8_t *) as->data + off;
	if (load) {
		memcpy (emu_mem, data, len);
	} else {
		memcpy (data, emu_mem, len);
	}
	return true;
}

bool CopytoEmu(Nsemu *nsemu, void *data, uint64_t addr, size_t len) {
	AddressSpace *as;
	if (!(as = FindAddressSpace (nsemu, addr, len))) {
		return false;
	}
	return _CopyMemEmu (as, data, addr, len, true);
}

bool CopytoEmuByName(Nsemu *nsemu, void *data, std::string name, size_t len) {
	if (nsemu->as.find (name) == nsemu->as.end ()) {
		return false;
	}
	AddressSpace *as = &nsemu->as[name];
	if (len > as->length) {
		return false;
	}
	return _CopyMemEmu (as, data, as->addr, len, true);
}

bool CopyfromEmu(Nsemu *nsemu, void *data, uint64_t addr, size_t len) {
	AddressSpace *as;
	if (!(as = FindAddressSpace (nsemu, addr, len))) {
		return false;
	}
	return _CopyMemEmu (as, data, addr, len, false);
}

bool CopyfromEmuByName(Nsemu *nsemu, void *data, std::string name, size_t len) {
	if (nsemu->as.find (name) == nsemu->as.end ()) {
		return false;
	}
	AddressSpace *as = &nsemu->as[name];
	if (len > as->length) {
		return false;
	}
	return _CopyMemEmu (as, data, as->addr, len, false);
}

}
