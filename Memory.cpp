/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include <sys/mman.h>
#include <unistd.h>
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
    abort();
}

namespace Memory
{
static AddressSpace mem_map[] =
{
  AddressSpace (".text", (uint64_t)nullptr, 0x10000, PROT_READ | PROT_EXEC),
  AddressSpace (".data", (uint64_t)nullptr, 0x10000, PROT_READ | PROT_WRITE),
};

bool InitMemmap (Nsemu *nsemu) {
  int num = sizeof(mem_map) / sizeof(AddressSpace);
  for (int n = 0; n < num; n++) {
    std::string sec_name = mem_map[n].GetName();
  }
}

}
