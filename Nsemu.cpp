/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
Nsemu *Nsemu::inst = nullptr;

static void load_nso(Nsemu *nsemu, string path, uint64_t addr) {
  Nso nso(path);
  nso.load(nsemu, addr);
}

bool Nsemu::BootApp(const std::string& path) {
  debug_print ("Booting... %s\n", path.c_str());
  load_nso(this, path, 0x1000);
  return true;
}
