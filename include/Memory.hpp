#ifndef _MEMORY_HPP
#define _MEMORY_HPP

#include <string>
#include <sys/types.h>

class AddressSpace {
private:
  std::string name;
  size_t length;
  void *data;
  int perm;
public:
  AddressSpace (std::string _name, uint64_t addr, size_t _length, int _perm);
  bool operator<(const AddressSpace &as) {
    return name < as.name;
  }
  std::string GetName() { return name; }
  void *GetPointer() { return data; }
};

#endif
