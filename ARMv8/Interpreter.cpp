/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

Interpreter *Interpreter::inst = nullptr;
IntprCallback *Interpreter::disas_cb = nullptr;

void Interpreter::Init() {
        Disassembler::Init();
}

int Interpreter::SingleStep() {
	uint32_t inst = ARMv8::ReadInst (PC);
	debug_print ("Run Code: 0x%lx: 0x%08lx\n", PC, inst);
        //ns_print ("Run Code: 0x%lx: 0x%08lx\n", PC, inst);
	Disassembler::DisasA64 (inst, disas_cb);
	PC += sizeof(uint32_t);
        X(GPR_ZERO) = 0; //Reset Zero register
	return 0;
}

void Interpreter::Run() {
	debug_print ("Running with Interpreter\n");
        static uint64_t counter = 0;
        uint64_t estimate = 3485900, mx = 100000;
        //uint64_t estimate = 0, mx = 3420000;
	while (Cpu::GetState () == Cpu::State::Running) {
		if (GdbStub::enabled) {
                        if (GdbStub::cont) {
                                SingleStep();
                        } else {
        			GdbStub::HandlePacket();
                                if (GdbStub::step) {
                                        SingleStep ();
                                        GdbStub::step = false;
                                        GdbStub::Trap(); // Notify SIGTRAP to gdb client
                                }
                        }
		} else {
		    if (counter >= estimate){
				Cpu::DumpMachine ();
		    }
                    //Cpu::DumpMachine ();
		    if (counter >= estimate + mx) {
		        break;
                     }
                     SingleStep ();
		    counter++;
		}
	}
}

/* ####### Callbacks ####### */
enum OpType{
        AL_TYPE_LSL,
        AL_TYPE_LSR,
        AL_TYPE_ASR,
        AL_TYPE_ROR,
        AL_TYPE_ADD,
        AL_TYPE_SUB,
        AL_TYPE_AND,
        AL_TYPE_OR,
        AL_TYPE_EOR,
        AL_TYPE_SDIV,
        AL_TYPE_UDIV,
        AL_TYPE_MUL,
};

const char *OpStrs[] = { "<<", ">>", ">>", "ROR", "+", "-", "&", "|", "^" };

static bool CondHold(unsigned int cond) {
        bool result = false;
        if (cond >> 1 == 0x0) {
                result = ((NZCV & Z_MASK) != 0);
        } else if (cond >> 1 == 0x1) {
                result = ((NZCV & C_MASK) != 0);
        } else if (cond >> 1 == 0x2) {
                result = ((NZCV & N_MASK) != 0);
        } else if (cond >> 1 == 0x3) {
                result = ((NZCV & V_MASK) != 0);
        } else if (cond >> 1 == 0x4) {
                result = ((NZCV & C_MASK) != 0) & ((NZCV & Z_MASK) == 0);
                //printf("nzcv = 0x%016lx, result=%d(%lx,%lx)\n", NZCV, result, (NZCV & C_MASK), !(NZCV & Z_MASK));
        } else if (cond >> 1 == 0x5) {
                result = ((NZCV & N_MASK) != 0) == ((NZCV & V_MASK) != 0);
        } else if (cond >> 1 == 0x6) {
                result = (((NZCV & N_MASK) != 0) == ((NZCV & V_MASK) != 0)) & ((NZCV & Z_MASK) == 0);
        } else if (cond >> 1 == 0x7) {
                return true;
        } else {
                ns_abort ("Unknown condition\n");
        }
        if (cond & 0x1)
                result = !result;
        return result;
}

static inline bool IsNegative32(uint32_t num) {
        return (num & (1UL << 31)) != 0;
}
static inline bool IsNegative64(uint64_t num) {
        return (num & (1UL << 63)) != 0;
}

static void UpdateFlag32(uint32_t res, uint32_t arg1, uint32_t arg2) {
        /* XXX: In ARMv8, nzcv flag is only updated when ADD/SUB/AND/BIC.
         * So following logic can work correctly(?)
         */
        uint32_t nzcv = 0;
        if (IsNegative32(res)) nzcv |= N_MASK; // N
        if (res == 0UL) nzcv |= Z_MASK; // Z
        if (((arg1 & arg2) + ((arg1 ^ arg2) >> 1)) >> 31) nzcv |= C_MASK; //C (half adder ((x & y) + ((x ^ y) >> 1)))
        if ((IsNegative32(arg1) && IsNegative32(arg2) && !IsNegative32(res)) ||
        (!IsNegative32(arg1) && !IsNegative32(arg2) && IsNegative32(res))) {
                        nzcv |= V_MASK; //V
        }
        NZCV = nzcv;
}
static void UpdateFlag64(uint64_t res, uint64_t arg1, uint64_t arg2) {
        /* XXX: In ARMv8, nzcv flag is only updated when ADD/SUB/AND/BIC.
         * So following logic can work correctly(?)
         */
        uint32_t nzcv = 0;
        if (IsNegative64(res)) nzcv |= N_MASK; // N
        if (res == 0ULL) nzcv |= Z_MASK; // Z
        if (((arg1 & arg2) + ((arg1 ^ arg2) >> 1)) >> 63) nzcv |= C_MASK; //C (half adder ((x & y) + ((x ^ y) >> 1)))
        if ((IsNegative64(arg1) && IsNegative64(arg2) && !IsNegative64(res)) ||
        (!IsNegative64(arg1) && !IsNegative64(arg2) && IsNegative64(res))) {
                        nzcv |= V_MASK; //V
        }
        NZCV = nzcv;
}

static uint64_t RotateRight(uint64_t val, uint64_t rot) {
        uint64_t left = (val & (1 << rot - 1)) << (64 - rot);
        return left | (val >> rot);
}

static uint64_t ReverseBit64(uint64_t x) {
        /* Assign the correct byte position.  */
        x = byte_swap64_uint(x);
        /* Assign the correct nibble position.  */
        x = ((x & 0xf0f0f0f0f0f0f0f0ull) >> 4)
          | ((x & 0x0f0f0f0f0f0f0f0full) << 4);
        /* Assign the correct bit position.  */
        x = ((x & 0x8888888888888888ull) >> 3)
          | ((x & 0x4444444444444444ull) >> 1)
          | ((x & 0x2222222222222222ull) << 1)
          | ((x & 0x1111111111111111ull) << 3);
        return x;
}

static uint64_t Udiv64(uint64_t arg1, uint64_t arg2) {
        if (arg2 == 0)
                return 0;
        return arg1 / arg2;
}

static int64_t Sdiv64(int64_t arg1, int64_t arg2) {
        if (arg2 == 0)
                return 0;
        if (arg1 == LLONG_MIN && arg2 == -1) {
                return LLONG_MIN;
        }
        return arg1 / arg2;
}

static void Umul64(uint64_t *res_h, uint64_t *res_l, uint64_t arg1, uint64_t arg2) {
        typedef union {
                uint64_t ll;
                struct {
                        uint32_t low, high;
                } l;
        } LL;
        LL rl, rm, rn, rh, a0, b0;
        uint64_t c;

        a0.ll = arg1;
        b0.ll = arg2;

        rl.ll = (uint64_t)a0.l.low * b0.l.low;
        rm.ll = (uint64_t)a0.l.low * b0.l.high;
        rn.ll = (uint64_t)a0.l.high * b0.l.low;
        rh.ll = (uint64_t)a0.l.high * b0.l.high;

        c = (uint64_t)rl.l.high + rm.l.low + rn.l.low;
        rl.l.high = c;
        c >>= 32;
        c = c + rm.l.high + rn.l.high + rh.l.low;
        rh.l.low = c;
        rh.l.high += (uint32_t)(c >> 32);

        *res_l = rl.ll;
        *res_h = rh.ll;
}

static void Smul64(uint64_t *res_h, uint64_t *res_l, uint64_t arg1, uint64_t arg2) {
        uint64_t rh;

        Umul64(res_l, &rh, arg1, arg2);

        /* Adjust for signs.  */
        if (arg2 < 0) {
                rh -= arg1;
        }
        if (arg1 < 0) {
                rh -= arg2;
        }
        *res_h = rh;
}

static uint64_t ALCalc(uint64_t arg1, uint64_t arg2, OpType op) {
        if (op == AL_TYPE_ADD)
                return arg1 + arg2;
        if (op == AL_TYPE_AND)
                return arg1 & arg2;
        if (op == AL_TYPE_OR)
                return arg1 | arg2;
        if (op == AL_TYPE_EOR)
                return arg1 ^ arg2;
        if (op == AL_TYPE_LSL)
                return arg1 << arg2;
        if (op == AL_TYPE_LSR)
                return arg1 >> arg2;
        if (op == AL_TYPE_ASR)
                return (int64_t) arg1 >> arg2;
        if (op == AL_TYPE_ROR)
                return RotateRight (arg1, arg2);
        if (op == AL_TYPE_UDIV) {
                return Udiv64 (arg1, arg2);
        }
        if (op == AL_TYPE_SDIV) {
                return Sdiv64 (arg1, arg2);
        }
        if (op == AL_TYPE_MUL) {
                return arg1 * arg2;
        }
        return 0;
}

static void ArithmeticLogic(unsigned int rd_idx, uint64_t arg1, uint64_t arg2, bool setflags, bool bit64, OpType _op) {
        uint64_t result;
        OpType op = _op;
        if (_op == AL_TYPE_SUB) {
                if (bit64) {
                        arg2 = -(int64_t) arg2;
                } else {
                        arg2 = -(int32_t) arg2;
                }
                op = AL_TYPE_ADD;
        }
        result = ALCalc (arg1, arg2, op);
        if (setflags) {
                if (bit64) {
                        UpdateFlag64 (result, arg1, arg2);
                        if (_op == AL_TYPE_SUB && arg2 == 0) {
                                NZCV |= C_MASK;
                        }
                } else {
                        UpdateFlag32 (result, arg1, arg2);
                }
        }
        if (bit64) {
                X(rd_idx) = result;
        } else {
                X(rd_idx) = result & 0xffffffff;
        }
}

void IntprCallback::MoviI64(unsigned int reg_idx, uint64_t imm, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = 0x%lx\n", regc, reg_idx, imm);
        X(reg_idx) = imm;
}

void IntprCallback::DepositI64(unsigned int rd_idx, uint64_t imm, unsigned int pos, unsigned int len, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOVK: %c[%u] = 0x%lx\n", regc, rd_idx, imm << pos);
        uint32_t mask = (1 << len) - 1; //XXX: hard coded bit size: 16
        X(rd_idx) = (X(rd_idx) & ~(mask << pos)) | (imm << pos);
}

void IntprCallback::DepositReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        if (bit64) {
                DepositI64 (rd_idx, X(rn_idx), pos, len, bit64);
        }
        else {
                DepositI64 (rd_idx, W(rn_idx), pos, len, bit64);
        }
}

void IntprCallback::DepositZeroI64(unsigned int rd_idx, uint64_t imm, unsigned int pos, unsigned int len, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("DepZ: %c[%u](pos:%u, len:%u) = 0x%lx\n", regc, rd_idx, pos, len, imm);
        X(rd_idx) = (imm & ((1ULL << len) - 1)) << pos;
}

void IntprCallback::DepositZeroReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        if (bit64) {
                DepositZeroI64 (rd_idx, X(rn_idx), pos, len, bit64);
        }
        else {
                DepositZeroI64 (rd_idx, W(rn_idx), pos, len, bit64);
        }
}

/* Mov between registers */
void IntprCallback::MovReg(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = %c[%u]\n", regc, rd_idx, regc, rn_idx);
        if (bit64)
                X(rd_idx) = X(rn_idx);
        else
                X(rd_idx) = W(rn_idx);
}

/* Conditional mov between registers */
void IntprCallback::CondMovReg(unsigned int cond, unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = 0: %c[%u], 1: %c[%u]\n", regc, rd_idx, regc, rm_idx, regc, rn_idx);
        if (bit64) {
                if (CondHold(cond))
                        X(rd_idx) = X(rn_idx);
                else
                        X(rd_idx) = X(rm_idx);
        } else {
                if (CondHold(cond))
                        X(rd_idx) = W(rn_idx);
                else
                        X(rd_idx) = W(rm_idx);
        }
}

/* Add/Sub with Immediate value */
void IntprCallback::AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
        if (!setflags) {
                rd_idx = ARMv8::HandleAsSP (rd_idx);
                rn_idx = ARMv8::HandleAsSP (rn_idx);
        }
	debug_print ("Add: %c[%u] = %c[%u] + 0x%lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), imm, setflags, bit64, AL_TYPE_ADD);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), imm, setflags, bit64, AL_TYPE_ADD);
}
void IntprCallback::SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
        if (!setflags) {
                rd_idx = ARMv8::HandleAsSP (rd_idx);
                rn_idx = ARMv8::HandleAsSP (rn_idx);
        }
	debug_print ("Sub: %c[%u] = %c[%u] - 0x%lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), imm, setflags, bit64, AL_TYPE_SUB);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), imm, setflags, bit64, AL_TYPE_SUB);
}

/* Add/Sub/Div/Shift between registers */
void IntprCallback::AddReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Add: %c[%u] = %c[%u] + %c[%u] (flag: %s)\n", regc, rd_idx, regc, rn_idx, regc, rm_idx, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), setflags, bit64, AL_TYPE_ADD);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), setflags, bit64, AL_TYPE_ADD);
}
void IntprCallback::SubReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Sub: %c[%u] = %c[%u] - %c[%u] (flag: %s)\n", regc, rd_idx, regc, rn_idx, regc, rm_idx, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), setflags, bit64, AL_TYPE_SUB);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), setflags, bit64, AL_TYPE_SUB);
}
void IntprCallback::MulReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign, bool dst64, bool src64) {
	char regdc = dst64? 'X': 'W';
	char regsc = src64? 'X': 'W';
	debug_print ("%cMUL: %c[%u] = %c[%u] * %c[%u] \n", sign?'S':'U', regdc, rd_idx, regsc, rn_idx, regsc, rm_idx);
        if (dst64) {
                if (src64) {
                        X(rd_idx) = X(rn_idx) * X(rm_idx);
                } else {
                        if (sign)
                                X(rd_idx) = (int64_t)((int32_t)W(rn_idx) * (int32_t)W(rm_idx));
                        else
                                X(rd_idx) = W(rn_idx) * W(rm_idx);
                }
        } else {
                X(rd_idx) = (W(rn_idx) * W(rm_idx)) & 0xffffffff;
        }
}
//64bit * 64bit
void IntprCallback::Mul2Reg(unsigned int rh_idx, unsigned int rl_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign) {
	debug_print ("%cMUL2: X[%u], X[%u] = X[%u] * X[%u] \n", sign?'S':'U', rh_idx, rl_idx, rn_idx, rm_idx);
        if (sign)
                Smul64(&X(rh_idx), &X(rl_idx), X(rn_idx), X(rm_idx));
        else
                Umul64(&X(rh_idx), &X(rl_idx), X(rn_idx), X(rm_idx));
}

void IntprCallback::DivReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("%cDIV: %c[%u] = %c[%u] / %c[%u] \n", sign?'S':'U', regc, rd_idx, regc, rn_idx, regc, rm_idx);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), false, bit64, sign?AL_TYPE_SDIV:AL_TYPE_UDIV);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), false, bit64, sign?AL_TYPE_SDIV:AL_TYPE_UDIV);
}
void IntprCallback::ShiftReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, unsigned int shift_type, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Shift: %c[%u] = %c[%u] %s %c[%u] \n", regc, rd_idx, regc, rn_idx, OpStrs[shift_type], regc, rm_idx);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), false, bit64, (OpType)shift_type);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), false, bit64, (OpType)shift_type);

}

/* Add/Sub with carry flag between registers */
void IntprCallback::AddcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
        AddReg (rd_idx, rn_idx, rm_idx, setflags, bit64);
        /* Add carry */
        if (NZCV & C_MASK) {
                if (bit64)
                        X(rd_idx) = X(rd_idx) + 1;
                else
                        X(rd_idx) = W(rd_idx) + 1;
        }
}
void IntprCallback::SubcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
        SubReg (rd_idx, rn_idx, rm_idx, setflags, bit64);
        /* Add carry */
        if (NZCV & C_MASK) {
                if (bit64)
                        X(rd_idx) = X(rd_idx) + 1;
                else
                        X(rd_idx) = W(rd_idx) + 1;
        }
}

/* AND/OR/EOR... with Immediate value */
void IntprCallback::AndI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("And: %c[%u] = %c[%u] & 0x%lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, wmask, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, setflags, bit64, AL_TYPE_AND);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, setflags, bit64, AL_TYPE_AND);

}
void IntprCallback::OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Or: %c[%u] = %c[%u] | 0x%lx \n", regc, rd_idx, regc, rn_idx, wmask);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, false, bit64, AL_TYPE_OR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, false, bit64, AL_TYPE_OR);
}
void IntprCallback::EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Eor: %c[%u] = %c[%u] ^ 0x%lx \n", regc, rd_idx, regc, rn_idx, wmask);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, false, bit64, AL_TYPE_EOR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, false, bit64, AL_TYPE_EOR);
}
void IntprCallback::ShiftI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int shift_type, unsigned int shift_amount, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Shift: %c[%u] = %c[%u] %s 0x%lx \n", regc, rd_idx, regc, rn_idx, OpStrs[shift_type], shift_amount);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), shift_amount, false, bit64, (OpType)shift_type);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), shift_amount, false, bit64, (OpType)shift_type);
}

/* AND/OR/EOR/BIC/NOT... between registers */
void IntprCallback::AndReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
        //rd_idx = ARMv8::HandleAsSP (rd_idx);
	debug_print ("And: %c[%u] = %c[%u] & %c[%u] (flag: %s)\n", regc, rd_idx, regc, rn_idx, regc, rm_idx, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), setflags, bit64, AL_TYPE_AND);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), setflags, bit64, AL_TYPE_AND);
}
void IntprCallback::OrrReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
        rd_idx = ARMv8::HandleAsSP (rd_idx);
	debug_print ("Or: %c[%u] = %c[%u] | %c[%u]\n", regc, rd_idx, regc, rn_idx, regc, rm_idx);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), false, bit64, AL_TYPE_OR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), false, bit64, AL_TYPE_OR);
}
void IntprCallback::EorReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
        rd_idx = ARMv8::HandleAsSP (rd_idx);
	debug_print ("Eor: %c[%u] = %c[%u] ^ %c[%u]\n", regc, rd_idx, regc, rn_idx, regc, rm_idx);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), false, bit64, AL_TYPE_EOR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), false, bit64, AL_TYPE_EOR);
}
void IntprCallback::BicReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("BIC: %c[%u] = %c[%u] & ~%c[%u]\n", regc, rd_idx, regc, rn_idx, regc, rm_idx);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), ~X(rm_idx), false, bit64, AL_TYPE_AND);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), ~W(rm_idx), false, bit64, AL_TYPE_AND);
}
void IntprCallback::NotReg(unsigned int rd_idx, unsigned int rm_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("NOT: %c[%u] = ~%c[%u]\n", regc, rd_idx, regc, rm_idx);
        if (bit64)
                X(rd_idx) = ~X(rm_idx);
        else
                X(rd_idx) = ~W(rm_idx);
}

static void ExtendRegI64(unsigned int rd_idx, uint64_t imm, unsigned int option) {
        int extsize = extract32(option, 0, 2);
        bool is_signed = extract32(option, 2, 1);
        if (is_signed) {
                switch (extsize) {
                case 0:
                        X(rd_idx) = (int64_t)(int8_t)(imm & 0xff);
                        break;
                case 1:
                        X(rd_idx) = (int64_t)(int16_t)(imm & 0xffff);
                        break;
                case 2:
                        X(rd_idx) = (int64_t)(int32_t)(imm & 0xffffffff);
                        break;
                case 3:
                        X(rd_idx) = imm;
                        break;
                }
        } else {
                switch (extsize) {
                case 0:
                        X(rd_idx) = (uint64_t) (imm & 0xff);
                        break;
                case 1:
                        X(rd_idx) = (uint64_t) (imm & 0xffff);
                        break;
                case 2:
                        X(rd_idx) = (uint64_t) (imm & 0xffffffff);
                        break;
                case 3:
                        X(rd_idx) = imm;
                        break;
                }
        }

}
void IntprCallback::ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Extend: %c[%u] Ext(%c[%u])\n", regc, rd_idx, regc, rn_idx);
        if (bit64)
                ExtendRegI64 (rd_idx, X(rn_idx), extend_type);
        else
                ExtendRegI64 (rd_idx, W(rn_idx), extend_type);
}

/* Load/Store */
void IntprCallback::_LoadReg(unsigned int rd_idx, uint64_t addr, int size, bool is_sign, bool extend) {
                debug_print ("Read from addr:0x%lx(%d)\n", addr, size);
                if (size == 0) {
                        X(rd_idx) = ARMv8::ReadU8 (addr);
                } else if (size == 1) {
                        X(rd_idx) = ARMv8::ReadU16 (addr);
                } else if (size == 2) {
                        X(rd_idx) = ARMv8::ReadU32 (addr);
                } else if (size == 3){
			X(rd_idx) = ARMv8::ReadU64 (addr);
                } else {
                        /* 128-bit Qt */
                        VREG(rd_idx).d[0] = ARMv8::ReadU64 (addr + 8);
                        VREG(rd_idx).d[1] = ARMv8::ReadU64 (addr);
                        //ns_debug("Read: Q = 0x%lx, 0x%lx\n", VREG(rd_idx).d[0], VREG(rd_idx).d[1]);
                }

		if (extend && is_sign) {
                        /* TODO: treat other cases (8bit-> or 16bit->)*/
                        X(rd_idx) = (int64_t) (int32_t) W(rd_idx);
                }
}

void IntprCallback::_StoreReg(unsigned int rd_idx, uint64_t addr, int size, bool is_sign, bool extend) {
                debug_print ("Write to addr:0x%lx(%d)\n", addr, size);
                if (size == 0) {
                        ARMv8::WriteU8 (addr, (uint8_t) (W(rd_idx) & 0xff));
                } else if (size == 1) {
                        ARMv8::WriteU16 (addr, (uint16_t) (W(rd_idx) & 0xffff));
                } else if (size == 2) {
                        ARMv8::WriteU32 (addr, W(rd_idx));
                } else if (size == 3) {
                        ARMv8::WriteU64 (addr, X(rd_idx));
                } else {
                        /* 128-bit Qt */
                        ARMv8::WriteU64 (addr + 8, VREG(rd_idx).d[0]);
                        ARMv8::WriteU64 (addr, VREG(rd_idx).d[1]);
                        //ns_debug("Write: Q = 0x%lx, 0x%lx\n", VREG(rd_idx).d[0], VREG(rd_idx).d[1]);
                }
}

void IntprCallback::LoadReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size,
                            bool is_sign, bool extend, bool post, bool bit64) {
	        char regc = bit64? 'X': 'W';
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
                base_idx = ARMv8::HandleAsSP (base_idx);
                debug_print ("Load(%d): %c[%u] <= [%c[%u], %c[%u]](extend:%d, sign:%d)\n", size, regdc, rd_idx, regc, base_idx, regc, rm_idx, extend, is_sign);
                uint64_t addr;
                if (bit64) {
                        if (post)
                                addr = X(base_idx);
                        else
                                addr = X(base_idx) + X(rm_idx);
                        _LoadReg (rd_idx, addr, size, is_sign, extend);
                } else {
                        if (post)
                                addr = W(base_idx);
                        else
                                addr = W(base_idx) + W(rm_idx);
                        _LoadReg (rd_idx, addr, size, is_sign, extend);
                }
}
void IntprCallback::LoadRegI64(unsigned int rd_idx, unsigned int ad_idx, int size,
                                bool is_sign, bool extend) {
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Load(%d): %c[%u] <= [0x%lx]\n", size, regdc, rd_idx, X(ad_idx));
                _LoadReg (rd_idx, X(ad_idx), size, is_sign, extend);
}
void IntprCallback::StoreReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size,
                                bool is_sign, bool extend, bool post, bool bit64) {
	        char regc = bit64? 'X': 'W';
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
                base_idx = ARMv8::HandleAsSP (base_idx);
	        debug_print ("Store(%d): %c[%u] => [%c[%u], %c[%u]](extend:%d, sign:%d)\n", size, regdc, rd_idx, regc, base_idx, regc, rm_idx, extend, is_sign);
                uint64_t addr;
                if (bit64) {
                        if (post)
                                addr = X(base_idx);
                        else
                                addr = X(base_idx) + X(rm_idx);
                        _StoreReg (rd_idx, addr, size, is_sign, extend);
                } else {
                        if (post)
                                addr = W(base_idx);
                        else
                                addr = W(base_idx) + W(rm_idx);
                        _StoreReg (rd_idx, addr, size, is_sign, extend);
                }
}
void IntprCallback::StoreRegI64(unsigned int rd_idx, unsigned int ad_idx, int size,
                                bool is_sign, bool extend) {
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Store(%d): %c[%u] => [0x%lx]\n", size, regdc, rd_idx, X(ad_idx));
                _StoreReg (rd_idx, X(ad_idx), size, is_sign, extend);
}

/* Bitfield Signed/Unsigned Extract... with Immediate value */
/*  X(d)<> = X(n)<off:64> */
void IntprCallback::SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        char regc = bit64? 'X': 'W';
	debug_print ("SExtract: %c[%u] <= SEXT[%c[%u]: %u, %u]\n", regc, rd_idx, regc, rn_idx, pos, len);
        if (pos + len == 64) {
                ShiftI64 (rd_idx, rn_idx, AL_TYPE_ASR, 64 - len, true);
                goto fin;
        }
        ShiftI64 (GPR_DUMMY, rn_idx, AL_TYPE_LSL, 64 - len - pos, true);
        ShiftI64 (rd_idx, GPR_DUMMY, AL_TYPE_ASR, 64 - len, true);
fin:
        if (!bit64) {
                X(rd_idx) = (int32_t) (X(rd_idx) & 0xffffffff);
        }
}
void IntprCallback::UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        char regc = bit64? 'X': 'W';
	debug_print ("UExtract: %c[%u] <= SEXT[%c[%u]: %u, %u]\n", regc, rd_idx, regc, rn_idx, pos, len);
        if (pos + len == 64) {
                ShiftI64 (rd_idx, rn_idx, AL_TYPE_LSR, 64 - len, true);
                goto fin;
        }
        ShiftI64 (GPR_DUMMY, rn_idx, AL_TYPE_LSL, 64 - len - pos, true);
        ShiftI64 (rd_idx, GPR_DUMMY, AL_TYPE_LSR, 64 - len, true);
fin:
        if (!bit64) {
                X(rd_idx) = X(rd_idx) & 0xffffffff;
        }
}

/* Reverse bit order */
void IntprCallback::RevBit(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = ReverseBit64 (X(rn_idx));
        else
                X(rd_idx) = ReverseBit64 (W(rn_idx));
}
/* Reverse byte order per 16bit */
void IntprCallback::RevByte16(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = byte_swap16_uint (X(rn_idx));
        else
                X(rd_idx) = byte_swap16_uint (W(rn_idx));
}
/* Reverse byte order per 32bit */
void IntprCallback::RevByte32(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = byte_swap32_uint (X(rn_idx));
        else
                X(rd_idx) = byte_swap32_uint (W(rn_idx));
}
/* Reverse byte order per 64bit */
void IntprCallback::RevByte64(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        X(rd_idx) = byte_swap64_uint (X(rn_idx));
}

static inline unsigned int Clz32(uint64_t val) {
        return val ? (unsigned int) __builtin_clz(val) : 32;
}
static inline unsigned int Clz64(uint64_t val) {
        return val ? (unsigned int) __builtin_clzll(val) : 64;
}

static inline unsigned int Cls32(uint64_t val) {
		#ifdef __APPLE__
				return 0; //TODO
		#else
        		return val ? (unsigned int) __builtin_clrsb(val) : 32;
		#endif
}
static inline unsigned int Cls64(uint64_t val) {
		#ifdef __APPLE__
				return 0; //TODO
		#else
        		return val ? (unsigned int) __builtin_clrsbll(val) : 64;
		#endif
}

/* Count Leading Zeros */
void IntprCallback::CntLeadZero(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = Clz64 (X(rn_idx));
        else
                X(rd_idx) = Clz32 (W(rn_idx));
}
/* Count Leading Signed bits */
void IntprCallback::CntLeadSign(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = Cls64 (X(rn_idx));
        else
                X(rd_idx) = Cls32 (W(rn_idx));
}

/* Conditional compare... with Immediate value */
void IntprCallback::CondCmpI64(unsigned int rn_idx, unsigned int imm, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64) {
        if (CondHold (cond)) {
                if (op) {
                        SubI64 (GPR_ZERO, rn_idx, imm, true, bit64);
                } else {
                        AddI64 (GPR_ZERO, rn_idx, imm, true, bit64);
                }
        } else {
                /* Set new nzcv */
                NZCV = (nzcv) << 28;
        }
}
/* Conditional compare... between registers */
void IntprCallback::CondCmpReg(unsigned int rn_idx, unsigned int rm_idx, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64) {
        if (CondHold (cond)) {
                if (op) {
                        SubReg (GPR_ZERO, rn_idx, rm_idx, true, bit64);
                } else {
                        AddReg (GPR_ZERO, rn_idx, rm_idx, true, bit64);
                }
        } else {
                /* Set new nzcv */
                NZCV = (nzcv) << 28;
        }
}

/* Go to Immediate address */
void IntprCallback::BranchI64(uint64_t imm) {
        debug_print ("Goto: 0x%lx\n", imm + 4);
        PC = imm;
}

/* Conditional Branch with Immediate value and jump to Immediate address */
void IntprCallback::BranchCondiI64(unsigned int cond, unsigned int rt_idx, uint64_t imm, uint64_t addr, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("CondCmp(%u): (%c[%u] cmp 0x%lx) => goto: 0x%lx\n", cond, regc, rt_idx, imm, addr);
        if (cond == Disassembler::CondType_EQ) {
                if (bit64) {
                        if (X(rt_idx) == imm) PC = addr;
                } else {
                        if (W(rt_idx) == imm) PC = addr;
                }
        }
        else if (cond == Disassembler::CondType_NE) {
                if (bit64) {
                        if (X(rt_idx) != imm) PC = addr;
                } else {
                        if (W(rt_idx) != imm) PC = addr;
                }
        }
}

/* Conditional Branch with NZCV flags */
void IntprCallback::BranchFlag(unsigned int cond, uint64_t addr) {
        if (CondHold (cond)) {
                PC = addr;
        }
}

/* Set PC with reg */
void IntprCallback::SetPCReg(unsigned int rt_idx) {
        PC = X(rt_idx) - 4;
        debug_print ("Goto: 0x%lx\n", PC + 4);
}

/* Super Visor Call */
void IntprCallback::SVC(unsigned int svc_num) {
        ns_print ("SVC: 0x%02x\n", svc_num);
        if (SVC::svc_handlers[svc_num])
                SVC::svc_handlers[svc_num]();
        else
                ns_print ("Invalid svc number: %u\n", svc_num);
}
/* Breakpoint exception */
void IntprCallback::BRK(unsigned int memo) {
        ns_print ("BRK: (0x%x)\n", memo);
        PC -= sizeof(uint32_t); //XXX
        GdbStub::Trap();
}

/* Read Vector register to FP register */
void IntprCallback::ReadVecReg(unsigned int rd_idx, unsigned int vn_idx, unsigned int index, int size) {
        if (size == 0) {
                D(rd_idx) = VREG(vn_idx).b[index];
        } else if (size == 1) {
                D(rd_idx) = VREG(vn_idx).h[index];
        } else if (size == 2) {
                D(rd_idx) = VREG(vn_idx).s[index];
        } else if (size == 3) {
                D(rd_idx) = VREG(vn_idx).d[index];
        }
}

template<typename T>
static void DupVecImm(unsigned int vd_idx, T imm, int size, int dstsize) {
        uint32_t i;
        int sec_size = sizeof(T) * 8;
        for (i = 0; i + sec_size <= dstsize; i += sec_size){
                if (size == 0)
                        VREG(vd_idx).b[i / sec_size] = imm;
                else if (size == 1)
                        VREG(vd_idx).h[i / sec_size] = imm;
                else if (size == 2)
                        VREG(vd_idx).s[i / sec_size] = imm;
                else
                        VREG(vd_idx).d[i / sec_size] = imm;
        }
}

void IntprCallback::DupVecImmI32(unsigned int vd_idx, uint32_t imm, int size, int dstsize) {
        DupVecImm(vd_idx, imm, size, dstsize);
}

void IntprCallback::DupVecImmI64(unsigned int vd_idx, uint64_t imm, int size, int dstsize) {
        DupVecImm(vd_idx, imm, size, dstsize);
}

/* Duplicate an element of vector register to new one */
void IntprCallback::DupVecReg(unsigned int vd_idx, unsigned int vn_idx, unsigned int index, int size, int dstsize) {
        if (size == 0) {
                DupVecImm(vd_idx, VREG(vn_idx).b[index], size, dstsize);
        } else if (size == 1) {
                DupVecImm(vd_idx, VREG(vn_idx).h[index], size, dstsize);
        } else if (size == 2) {
                DupVecImm(vd_idx, VREG(vn_idx).s[index], size, dstsize);
        } else if (size == 3) {
                DupVecImm(vd_idx, VREG(vn_idx).d[index], size, dstsize);
        }
}

/* Duplicate an general register into vector register */
void IntprCallback::DupVecRegFromGen(unsigned int vd_idx, unsigned int rn_idx, int size, int dstsize) {
        if (size == 0) {
                DupVecImm(vd_idx, (uint8_t) (X(rn_idx) & 0xff), size, dstsize);
        } else if (size == 1) {
                DupVecImm(vd_idx, (uint16_t) (X(rn_idx) & 0xffff), size, dstsize);
        } else if (size == 2) {
                DupVecImm(vd_idx, (uint32_t) (X(rn_idx) & 0xffffffff), size, dstsize);
        } else if (size == 3) {
                DupVecImm(vd_idx, X(rn_idx), size, dstsize);
        }
}

/* Read/Write Sysreg */
void IntprCallback::ReadWriteSysReg(unsigned int rd_idx, int offset, bool read) {
        uint64_t *sysr = (uint64_t *)((uint8_t *)&SYSR + offset);
        if (read) {
                X(rd_idx) = *sysr;
        } else {
                *sysr = X(rd_idx);
        }
}

/* Read/Write NZCV */
void IntprCallback::ReadWriteNZCV(unsigned int rd_idx, bool read) {
        if (read) {
                X(rd_idx) = NZCV;
        } else {
                NZCV = (uint32_t)(X(rd_idx) & 0xffffffff);
        }
}

/* Fp Mov between registers */
void IntprCallback::FMovReg(unsigned int fd_idx, unsigned int fn_idx, int type) {
        if (type == 0) {
                S(fd_idx) = S(fn_idx);
        } else if (type == 1) {
                D(fd_idx) = D(fn_idx);
        } else if (type == 3) {
                H(fd_idx) = H(fn_idx);
        }

}
