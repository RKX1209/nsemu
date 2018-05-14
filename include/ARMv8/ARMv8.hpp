#ifndef _ARMV8_HPP
#define _ARMV8_HPP

namespace ARMv8 {

typedef union {
        uint32_t w[2];
        uint64_t x;
}reg_t;

/* NEON & FP register */
typedef union {
        uint8_t  b[16];
        uint16_t h[8];
        uint32_t s[4];
        uint64_t d[2];
}vreg_t;

struct ARMv8State {
        /*
         * x0 - x31 (x30 is usually "link regsiter" and x31 is "stack pointer" or "zero register" )
         * NOTE: In nsemu, 'PC' register is respresented as x32 internally.
         */
	reg_t gpr[35];

        /* v0 - v31 (128 bit vector register, sometime treated as set of smaller size regs) */
        vreg_t vreg[33];

        uint32_t nzcv;  // flag register

        /* System register */
        struct SysReg {
                union {
                        uint64_t tpidr_el[4];
                };
                uint64_t tpidrro_el[1];
                uint64_t tczid_el[1];
        } sysr;
};

extern ARMv8State arm_state;

#define GPR_LR          30
/* Zero register share the same encoding as SP register.
 * In emulator, it's not necessary to follow it.
 */
#define GPR_ZERO          31
#define GPR_SP            32
#define PC_IDX            33  // XXX: bit tricky
#define GPR_DUMMY         34
#define VREG_DUMMY        32

#define GPR(r) ARMv8::arm_state.gpr[r]
#define VREG(r) ARMv8::arm_state.vreg[r] // SIMD registers
#define SYSR (ARMv8::arm_state.sysr)

#define X(r) GPR(r).x
#define W(r) GPR(r).w[0]

/* FP registers */
#define D(r) VREG(r).d[0]
#define S(r) VREG(r).s[0]
#define H(r) VREG(r).h[0]
#define B(r) VREG(r).b[0]

#define LR X(GPR_LR)
#define SP X(GPR_SP)
#define ZERO X(GPR_ZERO)
#define PC X(PC_IDX)

#define NZCV ARMv8::arm_state.nzcv
#define N_MASK          0x80000000UL
#define Z_MASK          0x40000000UL
#define C_MASK          0x20000000UL
#define V_MASK          0x10000000UL

/* See: C6.1.3 Use of the stack pointer */
inline unsigned int HandleAsSP(unsigned r_idx) {
        return r_idx == GPR_ZERO ? GPR_SP : r_idx;
}

void Init();

void RunLoop();

void Dump();

void DumpJson(FILE *fp);

uint64_t GetTls();

}
#endif
