/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include <thread>
#include "Nsemu.hpp"

Nsemu *Nsemu::inst = nullptr;
static std::thread cpu_thread;

static void LoadNso(Nsemu *nsemu, string path) {
	Nso nso (path);
	nso.load (nsemu);
}

static void CpuThread() {
        ns_print ("[CPU]\tLaunching ARMv8::VCPU.....\n");
	Cpu::Init ();
	Cpu::SetState (Cpu::State::Running);
        ns_print ("[CPU]\tRunning.....\n");
	Cpu::Run ();
}

bool Nsemu::BootUp(const std::string& path) {
	ns_print ("Booting... %s\n", path.c_str ());
	Memory::InitMemmap (this);
	LoadNso (this, path);
        IPC::InitIPC();
	cpu_thread = std::thread (CpuThread);
	/* Run cpu */
	cpu_thread.join ();
	return true;
}
