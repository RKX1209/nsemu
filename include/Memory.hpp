#ifndef _MEMORY_HPP
#define _MEMORY_HPP

/* Memory utilities that support access by host virtual address. */

#include <string>
#include <sys/types.h>

class RAMBlock {
public:
std::string name;
size_t length;
int perm;
uint64_t addr; //gpa (guest physical address)
RAMBlock() {}
RAMBlock(std::string _name, uint64_t _addr, size_t _length, int _perm);
bool operator<(const RAMBlock &as) {
	return name < as.name;
}
};

class Nsemu;
namespace Memory
{
extern uint8_t *pRAM;	// XXX: Replace raw pointer to View wrapper.
extern uint64_t heap_base;
extern uint64_t heap_size;

void InitMemmap(Nsemu *nsemu);
RAMBlock *FindRAMBlock(Nsemu *nsemu, uint64_t addr, size_t len);
std::list<std::tuple<uint64_t,uint64_t, int>> GetRegions();
bool CopytoEmu(Nsemu *nsemu, void *data, uint64_t addr, size_t len);
bool CopytoEmuByName(Nsemu *nsemu, void *data, std::string name, size_t len);
bool CopyfromEmu(Nsemu *nsemu, void *data, uint64_t addr, size_t len);
bool CopyfromEmuByName(Nsemu *nsemu, void *data, std::string name, size_t len);

}
#endif
