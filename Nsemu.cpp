/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include <thread>
#include "Nsemu.hpp"

Nsemu *Nsemu::inst = nullptr;
static std::thread cpu_thread;

static void load_nso(Nsemu *nsemu, string path, uint64_t addr) {
  Nso nso(path);
  nso.load(nsemu, addr);
}

static void CpuThread() {
  Cpu::Init();
  Cpu::SetState(Cpu::State::Running);
  Cpu::Run();
}

bool Nsemu::BootUp(const std::string& path) {
  debug_print ("Booting... %s\n", path.c_str());
  Memory::InitMemmap(this);
  load_nso(this, path, 0x1000);
  cpu_thread = std::thread (CpuThread);  
  /* Run cpu */
  cpu_thread.join();
  return true;
}
