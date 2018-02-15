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
	/*while (Cpu::GetState () == Cpu::State::Running) {
		SingleStep ();
	}*/
        int test_max = 20;
        for (int i = 0; i < test_max; i++)
                SingleStep();
}

void IntprCallback::MoviI64(unsigned int reg_idx, uint64_t imm, bool unchanged, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV%c: %c[%u] = 0x%016lx\n", unchanged? 'K' : ' ', regc, reg_idx, imm);
}

/* Mov between registers */
void IntprCallback::MovReg(unsigned int rd_idx, unsigned int rn_idx, bool bit64) {

}

/* Add/Sub with Immediate value */
void IntprCallback::AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Add: %c[%u] = %c[%u] + 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
}
void IntprCallback::SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Sub: %c[%u] = %c[%u] - 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
}

/* Add/Sub between registers */
void IntprCallback::AddReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {}
void IntprCallback::SubReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {}

/* AND/OR/EOR... with Immediate value */
void IntprCallback::AndI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64) {}
void IntprCallback::OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {}
void IntprCallback::EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {}
void IntprCallback::ShiftI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int shift_type, unsigned int shift_amount, bool bit64) {}

/* AND/OR/EOR/BIC/NOT... between registers */
void IntprCallback::AndReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {}
void IntprCallback::OrrReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {}
void IntprCallback::EorReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) {}
void IntprCallback::BicReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) {}
void IntprCallback::NotReg(unsigned int rd_idx, unsigned int rm_idx, bool bit64) {}
void IntprCallback::ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64) {}

/* Bitfield Signed/Unsigned Extract... with Immediate value */
void IntprCallback::SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {}
void IntprCallback::UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {}

/* Go to Immediate address */
void IntprCallback::BranchI64(uint64_t imm) {
        debug_print ("Goto: 0x%016lx\n", imm + 4);
        PC = imm;
}

/* Conditional Branch with Immediate value and jump to Immediate address */
void IntprCallback::BranchCondiI64(unsigned int cond, unsigned int rt_idx, uint64_t imm, uint64_t addr, bool bit64) {

}

/* Conditional Branch with NZCV flags */
void IntprCallback::BranchFlag(unsigned int cond, uint64_t addr) {

}

/* Set PC with reg */
void IntprCallback::SetPCReg(unsigned int rt_idx) {

}

/* Super Visor Call */
void IntprCallback::SVC(unsigned int svc_num) {
        debug_print ("SVC: %u\n", svc_num);
}
