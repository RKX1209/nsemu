#ifndef _MEMORY_HPP
#define _MEMORY_HPP

/* Memory utilities that support access by host virtual address. */

#include <string>
#include <sys/types.h>

class RAMBlock {
public:
std::string name;
unsigned int length;
int perm;
uint64_t addr; //gpa (guest physical address)
uint8_t *block;
RAMBlock() { block = nullptr; }
RAMBlock(std::string _name, uint64_t _addr, unsigned int _length, int _perm); //straight mapping
RAMBlock(std::string _name, uint64_t _addr, unsigned int _length, uint8_t *raw, int _perm);
~RAMBlock() {
        if (block) {
                delete[] block;
        }
}
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
void AddMemmap(uint64_t addr, unsigned int len);
void DelMemmap(uint64_t addr, unsigned int len);
std::list<std::tuple<uint64_t,uint64_t, int>> GetRegions();

void *GetRawPtr(uint64_t gpa, unsigned int len);
bool CopytoEmu(Nsemu *nsemu, void *data, uint64_t addr, unsigned int len);
bool CopytoEmuByName(Nsemu *nsemu, void *data, std::string name, unsigned int len);
bool CopyfromEmu(Nsemu *nsemu, void *data, uint64_t addr, unsigned int len);

}
#endif
