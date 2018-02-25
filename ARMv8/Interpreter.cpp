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
                scanf("%c", &c);
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

void IntprCallback::MoviI64(unsigned int reg_idx, uint64_t imm, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV: %c[%u] = 0x%016lx\n", regc, reg_idx, imm);
        if (bit64) X(reg_idx) = imm;
        else W(reg_idx) = imm;
}

void IntprCallback::DepositiI64(unsigned int reg_idx, unsigned int pos, uint64_t imm, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOVK: %c[%u] = 0x%016lx\n", regc, reg_idx, imm << pos);
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

/* Add/Sub with Immediate value */
void IntprCallback::AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Add: %c[%u] = %c[%u] + 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), imm, setflags, bit64, AL_TYPE_ADD);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), imm, setflags, bit64, AL_TYPE_ADD);
}
void IntprCallback::SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Sub: %c[%u] = %c[%u] - 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), imm, setflags, bit64, AL_TYPE_SUB);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), imm, setflags, bit64, AL_TYPE_SUB);
}

/* Add/Sub between registers */
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
	debug_print ("And: %c[%u] = %c[%u] & 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, wmask, setflags? "update": "no");
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, setflags, bit64, AL_TYPE_AND);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, setflags, bit64, AL_TYPE_AND);

}
void IntprCallback::OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Or: %c[%u] = %c[%u] | 0x%016lx \n", regc, rd_idx, regc, rn_idx, wmask);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, false, bit64, AL_TYPE_OR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, false, bit64, AL_TYPE_OR);
}
void IntprCallback::EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Eor: %c[%u] = %c[%u] ^ 0x%016lx \n", regc, rd_idx, regc, rn_idx, wmask);
        if (bit64)
                ArithmeticLogic (rd_idx, X(rn_idx), wmask, false, bit64, AL_TYPE_EOR);
        else
                ArithmeticLogic (rd_idx, W(rn_idx), wmask, false, bit64, AL_TYPE_EOR);
}
void IntprCallback::ShiftI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int shift_type, unsigned int shift_amount, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Shift: %c[%u] = %c[%u] %s 0x%016lx \n", regc, rd_idx, regc, rn_idx, OpStrs[shift_type], shift_amount);
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
        if (bit64)
                X(rd_idx) = ~X(rm_idx);
        else
                W(rd_idx) = ~W(rm_idx);
}
void IntprCallback::ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64) {
        /* TODO: */
}

/* Bitfield Signed/Unsigned Extract... with Immediate value */
void IntprCallback::SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        /* TODO: */
}
void IntprCallback::UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {
        /* TODO: */
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
        debug_print ("Goto: 0x%016lx\n", imm + 4);
        PC = imm;
}

/* Conditional Branch with Immediate value and jump to Immediate address */
void IntprCallback::BranchCondiI64(unsigned int cond, unsigned int rt_idx, uint64_t imm, uint64_t addr, bool bit64) {
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
