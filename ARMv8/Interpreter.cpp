/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

Interpreter *Interpreter::inst = nullptr;
IntprCallback *Interpreter::disas_cb = nullptr;

int Interpreter::SingleStep() {
	uint32_t inst = byte_swap32_uint (ARMv8::ReadInst (PC));
	debug_print ("Run Code: 0x%lx: 0x%08lx\n", PC, inst);
	Disassembler::DisasA64 (inst, disas_cb);
	PC += sizeof(uint32_t);
	return 0;
}

void Interpreter::Run() {
	debug_print ("Running with Interpreter\n");
	while (Cpu::GetState () == Cpu::State::Running) {
                char c;
                //scanf("%c", &c);
		SingleStep ();
                Cpu::DumpMachine ();
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
};

const char *OpStrs[] = { "<<", ">>", ">>", "ROR", "+", "-", "&", "|", "^" };

static bool CondHold(unsigned int cond) {
        cond >>= 1;
        if (cond == 0x0)
                return NZCV & Z_MASK;
        if (cond == 0x1)
                return NZCV & C_MASK;
        if (cond == 0x2)
                return NZCV & N_MASK;
        if (cond == 0x3)
                return NZCV & V_MASK;
        if (cond == 0x4)
                return (NZCV & C_MASK) & !(NZCV & Z_MASK) ;
        if (cond == 0x5)
                return (NZCV & N_MASK) == (NZCV & V_MASK);
        if (cond == 0x6)
                return ((NZCV & N_MASK) == (NZCV & V_MASK)) & !(NZCV & Z_MASK);
        if (cond == 0x7)
                return true;
        ns_abort ("Unknown condition\n");
        return false;
}

static void UpdateFlag(uint64_t res, uint64_t arg1, uint64_t arg2) {
        uint32_t nzcv;
        if (res & (1ULL << 63)) nzcv |= N_MASK; // N
        if (res) nzcv |= Z_MASK; // Z
        if (((arg1 & arg2) + (arg1 ^ arg2) >> 1) >> 63) nzcv |= C_MASK; //C (half adder ((x & y) + ((x ^ y) >> 1)))
        if ((arg1 ^ arg2) & (arg2 ^ res)) nzcv |= V_MASK; //V
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

static uint64_t ALCalc(uint64_t arg1, uint64_t arg2, OpType op) {
        if (op == AL_TYPE_ADD)
                return arg1 + arg2;
        if (op == AL_TYPE_SUB)
                return arg1 - arg2;
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
        return 0;
}

static void ArithmeticLogic(unsigned int rd_idx, uint64_t arg1, uint64_t arg2, bool setflags, bool bit64, OpType op) {
        uint64_t result;
        if (bit64) {
                result = ALCalc (arg1, arg2, op);
                if (setflags)
                        UpdateFlag (result, arg1, arg2);
                X(rd_idx) = result;
        }
        else {
                result = ALCalc (arg1, arg2, op);
                if (setflags)
                        UpdateFlag (result, arg1, arg2);
                W(rd_idx) = result;
        }
}

void IntprCallback::MoviI64(unsigned int reg_idx, uint64_t imm, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = 0x%lx\n", regc, reg_idx, imm);
        if (bit64) X(reg_idx) = imm;
        else W(reg_idx) = imm;
}

void IntprCallback::DepositiI64(unsigned int reg_idx, unsigned int pos, uint64_t imm, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOVK: %c[%u] = 0x%lx\n", regc, reg_idx, imm << pos);
        uint32_t mask = (1 << 16) - 1; //XXX: hard coded bit size: 16
        if (bit64) {
                X(reg_idx) = (X(reg_idx) & ~(mask << pos)) | (imm << pos);
        }
        else {
                W(reg_idx) = (W(reg_idx) & ~(mask << pos)) | (imm << pos);
        }
}

/* Mov between registers */
void IntprCallback::MovReg(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = %c[%u]\n", regc, rd_idx, regc, rn_idx);
        if (bit64)
                X(rd_idx) = X(rn_idx);
        else
                W(rd_idx) = W(rn_idx);
}

/* Conditional mov between registers */
void IntprCallback::CondMovReg(unsigned int cond, unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = %c[%u]\n", regc, rd_idx, regc, rn_idx);
        if (bit64) {
                if (CondHold(cond))
                        X(rd_idx) = X(rn_idx);
        } else {
                if (CondHold(cond))
                        W(rd_idx) = W(rn_idx);
        }
}

/* Add/Sub with Immediate value */
void IntprCallback::AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Add: %c[%u] = %c[%u] + 0x%lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), imm, setflags, bit64, AL_TYPE_ADD);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), imm, setflags, bit64, AL_TYPE_ADD);
}
void IntprCallback::SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
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
                        W(rd_idx) = W(rd_idx) + 1;
        }
}
void IntprCallback::SubcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {
        SubReg (rd_idx, rn_idx, rm_idx, setflags, bit64);
        /* Add carry */
        if (NZCV & C_MASK) {
                if (bit64)
                        X(rd_idx) = X(rd_idx) + 1;
                else
                        W(rd_idx) = W(rd_idx) + 1;
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
	debug_print ("And: %c[%u] = %c[%u] & %c[%u] (flag: %s)\n", regc, rd_idx, regc, rn_idx, regc, rm_idx, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), setflags, bit64, AL_TYPE_AND);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), setflags, bit64, AL_TYPE_AND);
}
void IntprCallback::OrrReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Or: %c[%u] = %c[%u] | %c[%u]\n", regc, rd_idx, regc, rn_idx, regc, rm_idx);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), X(rm_idx), false, bit64, AL_TYPE_OR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), W(rm_idx), false, bit64, AL_TYPE_OR);
}
void IntprCallback::EorReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
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
                W(rd_idx) = ~W(rm_idx);
}
void IntprCallback::ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64) {
        /* TODO: */
}

/* Load/Store */
static void _LoadReg(unsigned int rd_idx, uint64_t addr, int size, bool extend) {
                debug_print ("Read from addr:0x%lx(%d)\n", addr, size);
		if (size < 3) {
			W(rd_idx) = ARMv8::ReadU32 (addr);
                } else if (size < 4){
			X(rd_idx) = ARMv8::ReadU64 (addr);
                } else {
                        /* 128-bit Qt */
                        VREG(rd_idx).d[0] = ARMv8::ReadU64 (addr + 8);
                        VREG(rd_idx).d[1] = ARMv8::ReadU64 (addr);
                }

		/* TODO: if (extend)
				ExtendReg(rd_idx, rd_idx, type, true); */
}

static void _StoreReg(unsigned int rd_idx, uint64_t addr, int size, bool extend) {
                debug_print ("Write to addr:0x%lx(%d)\n", addr, size);
		if (size < 3) {
                        ARMv8::WriteU32 (addr, W(rd_idx));
                } else if (size < 4) {
                        ARMv8::WriteU64 (addr, X(rd_idx));
                } else {
                        /* 128-bit Qt */
                        ARMv8::WriteU64 (addr + 8, VREG(rd_idx).d[0]);
                        ARMv8::WriteU64 (addr, VREG(rd_idx).d[1]);
                }
		/* TODO: if (extend)
				ExtendReg(rd_idx, rd_idx, type, true); */
}

void IntprCallback::LoadReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size,
                            bool extend, bool post, bool bit64) {
	        char regc = bit64? 'X': 'W';
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Load(%d): %c[%u] <= [%c[%u], %c[%u]]\n", size, regdc, rd_idx, regc, base_idx, regc, rm_idx);
                uint64_t addr;
                if (bit64) {
                        if (post)
                                addr = X(base_idx);
                        else
                                addr = X(base_idx) + X(rm_idx);
                        _LoadReg (rd_idx, addr, size, extend);
                } else {
                        if (post)
                                addr = W(base_idx);
                        else
                                addr = W(base_idx) + W(rm_idx);
                        _LoadReg (rd_idx, addr, size, extend);
                }
}
void IntprCallback::LoadRegI64(unsigned int rd_idx, unsigned int base_idx, uint64_t offset, int size,
                                bool extend, bool post) {
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Load(%d): %c[%u] <= [X[%u], 0x%lx]\n", size, regdc, rd_idx, base_idx, offset);
                uint64_t addr;
                if (post)
                        addr = X(base_idx);
                else
                        addr = X(base_idx) + offset;
                _LoadReg (rd_idx, addr, size, extend);
}
void IntprCallback::StoreReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size,
                                bool extend, bool post, bool bit64) {
	        char regc = bit64? 'X': 'W';
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Store(%d): %c[%u] => [%c[%u], %c[%u]]\n", size, regdc, rd_idx, regc, base_idx, regc, rm_idx);
                uint64_t addr;
                if (bit64) {
                        if (post)
                                addr = X(base_idx);
                        else
                                addr = X(base_idx) + X(rm_idx);
                        _StoreReg (rd_idx, addr, size, extend);
                } else {
                        if (post)
                                addr = W(base_idx);
                        else
                                addr = W(base_idx) + W(rm_idx);
                        _StoreReg (rd_idx, addr, size, extend);
                }
}
void IntprCallback::StoreRegI64(unsigned int rd_idx, unsigned int base_idx, uint64_t offset, int size,
                                        bool extend, bool post) {
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Store(%d): %c[%u] => [X[%u], 0x%lx]\n", size, regdc, rd_idx, base_idx, offset);
                uint64_t addr;
                if (post)
                        addr = X(base_idx);
                else
                        addr = X(base_idx) + offset;
                _StoreReg (rd_idx, addr, size, extend);
}

/* Bitfield Signed/Unsigned Extract... with Immediate value */
void IntprCallback::SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        /* TODO: */
}
void IntprCallback::UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        /* TODO: */
}

/* Reverse bit order */
void IntprCallback::RevBit(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = ReverseBit64 (X(rn_idx));
        else
                W(rd_idx) = ReverseBit64 (W(rn_idx));
}
/* Reverse byte order per 16bit */
void IntprCallback::RevByte16(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = byte_swap16_uint (X(rn_idx));
        else
                W(rd_idx) = byte_swap16_uint (W(rn_idx));
}
/* Reverse byte order per 32bit */
void IntprCallback::RevByte32(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = byte_swap32_uint (X(rn_idx));
        else
                W(rd_idx) = byte_swap32_uint (W(rn_idx));
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
                W(rd_idx) = Clz32 (W(rn_idx));
}
/* Count Leading Signed bits */
void IntprCallback::CntLeadSign(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
        if (bit64)
                X(rd_idx) = Cls64 (X(rn_idx));
        else
                W(rd_idx) = Cls32 (W(rn_idx));
}

/* Conditional compare... with Immediate value */
void IntprCallback::CondCmpI64(unsigned int rn_idx, unsigned int imm, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64) {
        if (CondHold (cond)) {
                if (op) {
                        if (bit64)
                                ArithmeticLogic (GPR_DUMMY, X(rn_idx), imm, true, bit64, AL_TYPE_SUB);
                        else
                                ArithmeticLogic (GPR_DUMMY, W(rn_idx), imm, true, bit64, AL_TYPE_SUB);
                } else {
                        if (bit64)
                                ArithmeticLogic (GPR_DUMMY, X(rn_idx), imm, true, bit64, AL_TYPE_ADD);
                        else
                                ArithmeticLogic (GPR_DUMMY, W(rn_idx), imm, true, bit64, AL_TYPE_ADD);
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
                        if (bit64)
                                ArithmeticLogic (GPR_DUMMY, X(rn_idx), X(rm_idx), true, bit64, AL_TYPE_SUB);
                        else
                                ArithmeticLogic (GPR_DUMMY, W(rn_idx), W(rm_idx), true, bit64, AL_TYPE_SUB);
                } else {
                        if (bit64)
                                ArithmeticLogic (GPR_DUMMY, X(rn_idx), X(rm_idx), true, bit64, AL_TYPE_ADD);
                        else
                                ArithmeticLogic (GPR_DUMMY, W(rn_idx), W(rm_idx), true, bit64, AL_TYPE_ADD);
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
        if (CondHold (cond))
                PC = addr;
}

/* Set PC with reg */
void IntprCallback::SetPCReg(unsigned int rt_idx) {
        PC = X(rt_idx);
}

/* Super Visor Call */
void IntprCallback::SVC(unsigned int svc_num) {
        debug_print ("SVC: %u\n", svc_num);
}
