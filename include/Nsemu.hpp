#ifndef _NSEMU_HPP
#define _NSEMU_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <ios>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <tuple>
#include <map>
#include <unordered_map>
#include <memory>
#include <vector>

using namespace std;

#include "GdbStub.hpp"
#include "Memory.hpp"
#include "Util.hpp"
#include "NintendoObject.hpp"
#include "Cpu.hpp"
#include "Kernel.hpp"
#include "Svc.hpp"
#include "ARMv8/ARMv8.hpp"
#include "ARMv8/Disassembler.hpp"
#include "ARMv8/Interpreter.hpp"
#include "ARMv8/MMU.hpp"

class KObject {
public:
        KObject() {}
};

extern uint32_t handle_id;
extern std::unordered_map<uint32_t, KObject *> handles;

template<typename T> uint32_t NewHandle(T* obj) {
        handles[handle_id] = (KObject *)obj;
        ns_print("New Handle 0x%x\n", handle_id);
        return handle_id++;
}

#include "Ipc.hpp"
#include "Thread.hpp"

/* Global NSEMU singleton class .*/
class Nsemu {
private:
Nsemu() = default;
~Nsemu() = default;

static Nsemu *inst;
public:
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
