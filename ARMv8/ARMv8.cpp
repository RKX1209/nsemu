/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
static Interpreter *cpu_engine;
namespace ARMv8 {

ARMv8State arm_state;

void Init() {
	Interpreter::create ();
	cpu_engine = Interpreter::get_instance ();
        PC = 0x0;
        SP = 0x3100000;
}

void RunLoop() {
	cpu_engine->Run ();
}

void Dump() {
        for (int r = 0; r < GPR_DUMMY; r++) {
                if (!X(r))
                        continue;
                if (r == GPR_LR)
                        ns_print ("LR\t");
                else if (r == GPR_SP)
                        ns_print ("SP\t");
                else
                        ns_print ("X%d\t", r);
                ns_print ("0x%016lx\n", X(r));
        }
        ns_print ("PC\t0x%016lx\n", PC);
        ns_print ("NZCV\t0x%016lx\n", NZCV);
}

static uint64_t counter;
void DumpJson(FILE *fp) {
        if (counter >= 243)
                return;
        file_print (fp, "%lu : {\n", counter++);
        for (int r = 0; r <= GPR_DUMMY; r++) {
                if (r == GPR_LR && X(r))
                        file_print (fp, "\"X%d\" : \"0x%016lx\"", r, X(r) + 4);
                else
                        file_print (fp, "\"X%d\" : \"0x%016lx\"", r, X(r));
                if (r == GPR_DUMMY)
                        file_print (fp, "\n");
                else
                        file_print (fp, ",\n");
        }
        file_print (fp, "},\n");
}

}
