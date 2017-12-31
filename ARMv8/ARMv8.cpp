/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
static Interpreter *cpu_engine;
namespace ARMv8 {

ARMv8State arm_state;

void Init() {
	Interpreter ::create ();
	cpu_engine = Interpreter ::get_instance ();
}

void RunLoop() {
	cpu_engine->Run ();
}

}
