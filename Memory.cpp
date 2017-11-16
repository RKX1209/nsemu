/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include <sys/mman.h>
#include "Nsemu.hpp"

AddressSpace::AddressSpace (std::string _name, uint64_t addr, size_t _length, int _perm) {
  int page = getpagesize();
  name = _name;
  length = _length;
  perm = _perm;
  if (addr & (page - 1))
    addr = addr & ~(page - 1);
  data = mmap((void *)addr, length, perm, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (data == MAP_FAILED)
    ns_abort("Failed to mmap %s\n", name.c_str());
  addr = (uint64_t) data;
}

namespace Memory
{
static AddressSpace mem_map[] =
{
  AddressSpace (".text", (uint64_t)0x2000000, 0x1000000, PROT_READ | PROT_EXEC),
  AddressSpace (".rdata", (uint64_t)0x3000000, 0x1000000, PROT_READ),
  AddressSpace (".data", (uint64_t)0x4000000, 0x1000000, PROT_READ | PROT_WRITE),
};

void InitMemmap (Nsemu *nsemu) {
  int num = sizeof(mem_map) / sizeof(AddressSpace);
  for (int n = 0; n < num; n++) {
    std::string sec_name = mem_map[n].name;
    nsemu->as[sec_name] = mem_map[n];
  }
}

static void _CopytoEmu (AddressSpace *as, void *data, uint64_t addr, size_t len) {
  uint64_t off = addr - as->addr;
  memcpy ((uint8_t *)as->data + off, data, len);
}

bool CopytoEmu (Nsemu *nsemu, void *data, uint64_t addr, size_t len) {
  AddressSpace *as;
  std::map<std::string, AddressSpace>::iterator it = nsemu->as.begin();
  for (; it != nsemu->as.end(); it++) {
    as = &it->second;
    if (as->addr <= addr && addr + len <= as->addr + as->length) {
      goto found;
    }
  }
  return false;
found:
  _CopytoEmu (as, data, addr, len);
  return true;
}

bool CopytoEmuByName (Nsemu *nsemu, void *data, std::string name, size_t len) {
  AddressSpace *as = &nsemu->as[name];
  if (len > as->length)
    return false;
  _CopytoEmu (as, data, as->addr, len);
  return true;
}

}
