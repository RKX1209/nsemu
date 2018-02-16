#ifndef _ARMV8_HPP
#define _ARMV8_HPP

namespace ARMv8 {

typedef union {
        uint32_t w[2];
        uint64_t x;
}reg_t;

struct ARMv8State {
	reg_t gpr[32];	// x0 - x31 (x30 is usually "link regsiter" and x31 is "stack pointer" or "zero register" )
	uint64_t pc;
};

extern ARMv8State arm_state;

#define GPR_LR          30
#define GPR_SP          31
#define GPR_ZERO        31

#define LR ARMv8::arm_state.gpr[GPR_LR]
#define SP ARMv8::arm_state.gpr[GPR_SP]
#define ZERO ARMv8::arm_state.gpr[GPR_ZERO]
#define PC ARMv8::arm_state.pc

#define GPR(r) ARMv8::arm_state.gpr[r]
#define X(r) GPR(r).x
#define W(r) GPR(r).w[1]

void Init();

void RunLoop();

void Dump();

}

#endif
