#ifndef _NSEMU_HPP
#define _NSEMU_HPP

#include <stdint.h>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace std;

#include "Memory.hpp"
#include "Util.hpp"
#include "NintendoObject.hpp"
#include "Cpu.hpp"
#include "ARMv8/ARMv8.hpp"
#include "ARMv8/Disassembler.hpp"
#include "ARMv8/Interpreter.hpp"
#include "ARMv8/MMU.hpp"

/* Global NSEMU singleton class .*/
class Nsemu {
private:
Nsemu() = default;
~Nsemu() = default;

static Nsemu *inst;
public:
std::map<std::string, AddressSpace> as;
public:
Nsemu(const Nsemu&) = delete;
Nsemu& operator=(const Nsemu&) = delete;
Nsemu(Nsemu&&) = delete;
Nsemu& operator=(Nsemu&&) = delete;

static Nsemu *get_instance() {
	return inst;
}

static void create() {
	if (!inst) {
		inst = new Nsemu;
	}
}

static void destroy() {
	if (inst) {
		delete inst;
		inst = nullptr;
	}
}
bool BootUp(const std::string& path);
};
#endif
