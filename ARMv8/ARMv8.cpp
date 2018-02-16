/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
static Interpreter *cpu_engine;
namespace ARMv8 {

ARMv8State arm_state;

void Init() {
	Interpreter::create ();
	cpu_engine = Interpreter::get_instance ();
}

void RunLoop() {
	cpu_engine->Run ();
}

void Dump() {
        for (int r = 0; r < 32; r++) {
                if (r == GPR_LR)
                        debug_print ("LR\t");
                else if (r == GPR_SP)
                        debug_print ("SP\t");
                else
                        debug_print ("X%d\t", r);
                debug_print ("0x%016lx\n", X(r));
        }
        debug_print ("PC\t0x%016lx\n", PC);
        debug_print ("NZCV\t0x%016lx\n", NZCV);
}

}
