/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

Interpreter *Interpreter::inst = nullptr;
IntprCallback *Interpreter::disas_cb = nullptr;

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
	while (Cpu::GetState () == Cpu::State::Running) {
                char c;
                //scanf("%c", &c);
		SingleStep ();
                if (counter >= 241)
                        break;
                counter++;
                // if (PC == 0x2d54) {
		//         SingleStep ();
                //         Cpu::DumpMachine ();
                //         debug_print("Reach\n");
                //         break;
                // }
	}
        Cpu::DumpMachine ();
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

static void UpdateFlag(uint64_t res, uint64_t arg1, uint64_t arg2) {
        /* XXX: In ARMv8, nzcv flag is only updated when ADD/SUB/AND/BIC.
         * So following logic can work correctly(?)
         */
        uint32_t nzcv = 0;
        if (res & (1ULL << 63)) nzcv |= N_MASK; // N
        if (res == 0ULL) nzcv |= Z_MASK; // Z
        if (((arg1 & arg2) + ((arg1 ^ arg2) >> 1)) >> 63) nzcv |= C_MASK; //C (half adder ((x & y) + ((x ^ y) >> 1)))
        if (!(arg1 ^ arg2 && (1ULL < 63)) & (arg2 ^ res && (1ULL < 63))) nzcv |= V_MASK; //V
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
        // if (op == AL_TYPE_SUB)
        //         return arg1 - arg2;
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
        if (op == AL_TYPE_SUB) {
                arg2 = -arg2;
                op = AL_TYPE_ADD;
        }
        result = ALCalc (arg1, arg2, op);
        if (setflags)
                UpdateFlag (result, arg1, arg2);
        X(rd_idx) = result;
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
	//debug_print ("MOVK: %c[%u] = 0x%lx\n", regc, rd_idx, imm << pos);
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
void IntprCallback::CondMovReg(unsigned int cond, unsigned int rd_idx, unsigned int rn_idx, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = %c[%u]\n", regc, rd_idx, regc, rn_idx);
        if (bit64) {
                if (CondHold(cond))
                        X(rd_idx) = X(rn_idx);
        } else {
                if (CondHold(cond))
                        X(rd_idx) = W(rn_idx);
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
        rd_idx = ARMv8::HandleAsSP (rd_idx);
	debug_print ("And: %c[%u] = %c[%u] & 0x%lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, wmask, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, setflags, bit64, AL_TYPE_AND);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, setflags, bit64, AL_TYPE_AND);

}
void IntprCallback::OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {
	char regc = bit64? 'X': 'W';
        rd_idx = ARMv8::HandleAsSP (rd_idx);
	debug_print ("Or: %c[%u] = %c[%u] | 0x%lx \n", regc, rd_idx, regc, rn_idx, wmask);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, false, bit64, AL_TYPE_OR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, false, bit64, AL_TYPE_OR);
}
void IntprCallback::EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {
	char regc = bit64? 'X': 'W';
        rd_idx = ARMv8::HandleAsSP (rd_idx);
	debug_print ("Eor: %c[%u] = %c[%u] ^ 0x%lx \n", regc, rd_idx, regc, rn_idx, wmask);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, false, bit64, AL_TYPE_EOR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, false, bit64, AL_TYPE_EOR);
}
void IntprCallback::ShiftI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int shift_type, unsigned int shift_amount, bool bit64) {
	char regc = bit64? 'X': 'W';
        rd_idx = ARMv8::HandleAsSP (rd_idx);
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
                X(rd_idx) = ~W(rm_idx);
}
void IntprCallback::ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64) {
        /* TODO: */
}

/* Load/Store */
static void _LoadReg(unsigned int rd_idx, uint64_t addr, int size, bool extend) {
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

		/* TODO: if (extend)
				ExtendReg(rd_idx, rd_idx, type, true); */
}

static void _StoreReg(unsigned int rd_idx, uint64_t addr, int size, bool extend) {
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
		/* TODO: if (extend)
				ExtendReg(rd_idx, rd_idx, type, true); */
}

void IntprCallback::LoadReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size,
                            bool extend, bool post, bool bit64) {
	        char regc = bit64? 'X': 'W';
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
                base_idx = ARMv8::HandleAsSP (base_idx);
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
void IntprCallback::LoadRegI64(unsigned int rd_idx, unsigned int ad_idx, int size,
                                bool extend) {
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Load(%d): %c[%u] <= [0x%lx]\n", size, regdc, rd_idx, X(ad_idx));
                _LoadReg (rd_idx, X(ad_idx), size, extend);
}
void IntprCallback::StoreReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size,
                                bool extend, bool post, bool bit64) {
	        char regc = bit64? 'X': 'W';
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
                base_idx = ARMv8::HandleAsSP (base_idx);
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
void IntprCallback::StoreRegI64(unsigned int rd_idx, unsigned int ad_idx, int size,
                                        bool extend) {
                char regdc = size >= 4 ? 'Q' : (size < 3 ? 'W' : 'X');
	        debug_print ("Store(%d): %c[%u] => [0x%lx]\n", size, regdc, rd_idx, X(ad_idx));
                _StoreReg (rd_idx, X(ad_idx), size, extend);
}

/* Bitfield Signed/Unsigned Extract... with Immediate value */
/*  X(d)<> = X(n)<off:64> */
void IntprCallback::SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        char regc = bit64? 'X': 'W';
	debug_print ("SExtract: %c[%u] <= SEXT[%c[%u]: %u, %u]\n", regc, rd_idx, regc, rn_idx, pos, len);
        if (pos + len == 64) {
                ShiftI64 (rd_idx, rn_idx, AL_TYPE_ASR, 64 - len, bit64);
                return;
        }
        ShiftI64 (GPR_DUMMY, rn_idx, AL_TYPE_LSL, 64 - len - pos, bit64);
        ShiftI64 (rd_idx, GPR_DUMMY, AL_TYPE_ASR, 64 - len, true);
}
void IntprCallback::UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        char regc = bit64? 'X': 'W';
	debug_print ("UExtract: %c[%u] <= SEXT[%c[%u]: %u, %u]\n", regc, rd_idx, regc, rn_idx, pos, len);
        if (pos + len == 64) {
                ShiftI64 (rd_idx, rn_idx, AL_TYPE_LSR, 64 - len, bit64);
                return;
        }
        ShiftI64 (GPR_DUMMY, rn_idx, AL_TYPE_LSL, 64 - len - pos, bit64);
        ShiftI64 (rd_idx, GPR_DUMMY, AL_TYPE_LSR, 64 - len, true);
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
        PC = X(rt_idx);
}

/* Super Visor Call */
void IntprCallback::SVC(unsigned int svc_num) {
        //debug_print ("SVC: %u\n", svc_num);
        ns_print ("SVC: 0x%02x\n", svc_num);
        if (SVC::svc_handlers[svc_num])
                SVC::svc_handlers[svc_num]();
        else
                ns_print ("Invalid svc number: %u\n", svc_num);
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

/* Write to FP register */
void IntprCallback::WriteFpReg(unsigned int fd_idx, unsigned int fn_idx)  {
        D(fd_idx) = D(fn_idx);
}
void IntprCallback::WriteFpRegI64(unsigned int fd_idx, uint64_t imm) {
        D(fd_idx) = imm;
}
