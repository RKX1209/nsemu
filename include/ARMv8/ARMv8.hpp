#ifndef _ARMV8_HPP
#define _ARMV8_HPP

namespace ARMv8 {

typedef union {
        uint32_t w[2];
        uint64_t x;
}reg_t;

struct ARMv8State {
	reg_t gpr[34];
        /*
         * x0 - x31 (x30 is usually "link regsiter" and x31 is "stack pointer" or "zero register" )
         * NOTE: In nsemu, 'PC' register is respresented as x32 internally. */
        uint32_t nzcv;  // flag register
};

extern ARMv8State arm_state;

#define GPR_LR          30
#define GPR_SP          31
#define GPR_ZERO        31
#define PC_IDX          32
#define GPR_DUMMY       33

#define LR ARMv8::arm_state.gpr[GPR_LR]
#define SP ARMv8::arm_state.gpr[GPR_SP]
#define ZERO ARMv8::arm_state.gpr[GPR_ZERO]
#define PC ARMv8::arm_state.gpr[PC_IDX].x // XXX: bit tricky
#define NZCV ARMv8::arm_state.nzcv
#define N_MASK          0x80000000
#define Z_MASK          0x40000000
#define C_MASK          0x20000000
#define V_MASK          0x10000000

#define GPR(r) ARMv8::arm_state.gpr[r]
#define X(r) GPR(r).x
#define W(r) GPR(r).w[1]

void Init();

void RunLoop();

void Dump();

}

#endif
