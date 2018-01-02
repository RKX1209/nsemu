/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

Interpreter *Interpreter::inst = nullptr;
IntprCallback *Interpreter::disas_cb = nullptr;

int Interpreter::SingleStep() {
	uint32_t inst = byte_swap(ARMv8::ReadInst (PC));
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
        int test_max = 3;
        for (int i = 0; i < test_max; i++)
                SingleStep();
}

void IntprCallback::MoviI64(unsigned int reg_idx, uint64_t imm, bool unchanged, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("MOV%c: %c[%u] = 0x%016lx\n", unchanged? 'K' : ' ', regc, reg_idx, imm);
}

/* Add/Sub with Immediate value */
void IntprCallback::AddiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Add: %c[%u] = %c[%u] + 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
}
void IntprCallback::SubiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) {
	char regc = bit64? 'X': 'W';
	debug_print ("Sub: %c[%u] = %c[%u] + 0x%016lx (flag: %s)\n", regc, rd_idx, regc, rn_idx, imm, setflags? "update": "no");
}
void IntprCallback::AndiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64) {}
void IntprCallback::OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {}
void IntprCallback::EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) {}
void IntprCallback::SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {}
void IntprCallback::UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) {}
void IntprCallback::BranchI64(uint64_t imm) {
        debug_print ("Goto: 0x%016lx\n", imm + 4);
        PC = imm;
}
void IntprCallback::BranchCondiI64(unsigned int cond, unsigned int rt_idx, uint64_t imm, uint64_t addr, bool bit64) {

}
void IntprCallback::BranchFlag(unsigned int cond, uint64_t addr) {

}
void IntprCallback::SetPCReg(unsigned int rt_idx) {

}
void IntprCallback::SVC(unsigned int svc_num) {
        debug_print ("SVC: %u\n", svc_num);
}
