#ifndef _MEMORY_HPP
#define _MEMORY_HPP

#include <string>
#include <sys/types.h>

class AddressSpace {
public:
  std::string name;
  size_t length;
  int perm;
  void *data;
  uint64_t addr;
  AddressSpace() {}
  AddressSpace(std::string _name, uint64_t addr, size_t _length, int _perm);
  bool operator<(const AddressSpace &as) {
    return name < as.name;
  }
  void *GetPointer() { return data; }
};

class Nsemu;
namespace Memory
{
void InitMemmap (Nsemu *nsemu);
bool CopytoEmu (Nsemu *nsemu, void *data, uint64_t addr, size_t len);
bool CopytoEmuByName (Nsemu *nsemu, void *data, std::string name, size_t len);

}
#endif
