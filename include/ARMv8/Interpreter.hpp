#ifndef _INTERPRETER_HPP
#define _INTERPRETER_HPP

class IntprCallback : public DisasCallback {
public:
/* Mov with Immediate value */
void MoviI64(unsigned int reg_idx, uint64_t imm, bool bit64);

/* Deposit (i.e. distination register won't be changed) with Immediate value */
void DepositI64(unsigned int reg_idx, uint64_t imm, unsigned int pos, unsigned int len, bool bit64);
void DepositReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64);
void DepositZeroI64(unsigned int reg_idx, uint64_t imm, unsigned int pos, unsigned int len, bool bit64);
void DepositZeroReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64);

/* Mov between registers */
void MovReg(unsigned int rd_idx, unsigned int rn_idx, bool bit64);

/* Conditional mov between registers */
void CondMovReg(unsigned int cond, unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64);

/* Add/Sub with Immediate value */
void AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64);
void SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64);

/* Add/Sub/Mul/Div between registers */
void AddReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64);
void SubReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64);
void MulReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign, bool dst64, bool src64);
void Mul2Reg(unsigned int rh_idx, unsigned int rl_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign); //64bit * 64bit
void DivReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign, bool bit64);
void ShiftReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, unsigned int shift_type, bool bit64) ;

/* Add/Sub with carry flag between registers */
void AddcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64);
void SubcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64);

/* AND/OR/EOR/Shift ... with Immediate value */
void AndI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64);
void OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64);
void EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64);
void ShiftI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int shift_type, unsigned int shift_amount, bool bit64);

/* AND/OR/EOR/BIC/NOT ... between registers */
void AndReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64);
void OrrReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64);
void EorReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64);
void BicReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64);
void NotReg(unsigned int rd_idx, unsigned int rm_idx, bool bit64);
void ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64);

/* Load/Store */
void LoadReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size, bool is_sign, bool extend, bool post, bool bit64);
void LoadRegI64(unsigned int rd_idx, unsigned int ad_idx, int size, bool is_sign, bool extend);
void StoreReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size, bool is_sign, bool extend, bool post, bool bit64);
void StoreRegI64(unsigned int rd_idx, unsigned int ad_idx, int size, bool is_sign, bool extend);
void _LoadReg(unsigned int rd_idx, uint64_t addr, int size, bool is_sign, bool extend);
void _StoreReg(unsigned int rd_idx, uint64_t addr, int size, bool is_sign, bool extend);

/* Bitfield Signed/Unsigned Extract... with Immediate value */
void SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64);
void UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64);

/* Reverse bit order */
void RevBit(unsigned int rd_idx, unsigned int rn_idx, bool bit64);
/* Reverse byte order per 16bit */
void RevByte16(unsigned int rd_idx, unsigned int rn_idx, bool bit64);
/* Reverse byte order per 32bit */
void RevByte32(unsigned int rd_idx, unsigned int rn_idx, bool bit64);
/* Reverse byte order per 64bit */
void RevByte64(unsigned int rd_idx, unsigned int rn_idx, bool bit64);
/* Count Leading Zeros */
void CntLeadZero(unsigned int rd_idx, unsigned int rn_idx, bool bit64);
/* Count Leading Signed bits */
void CntLeadSign(unsigned int rd_idx, unsigned int rn_idx, bool bit64);

/* Conditional compare... with Immediate value */
void CondCmpI64(unsigned int rn_idx, unsigned int imm, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64);
/* Conditional compare... between registers */
void CondCmpReg(unsigned int rn_idx, unsigned int rm_idx, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64);

/* Go to Immediate address */
void BranchI64(uint64_t imm);

/* Conditional Branch with Immediate value and jump to Immediate address */
void BranchCondiI64(unsigned int cond, unsigned int rt_idx, uint64_t imm, uint64_t addr, bool bit64);

/* Conditional Branch with NZCV flags */
void BranchFlag(unsigned int cond, uint64_t addr);

/* Set PC with reg */
void SetPCReg(unsigned int rt_idx);

/* Super Visor Call */
void SVC(unsigned int svc_num);
/* Breakpoint exception */
void BRK(unsigned int memo);

/* Read/Write Sysreg */
void ReadWriteSysReg(unsigned int rd_idx, int offset, bool read);
/* Read/Write NZCV */
void ReadWriteNZCV(unsigned int rd_idx, bool read);

/* Fp Mov between registers */
void FMovReg(unsigned int fd_idx, unsigned int fn_idx, int type);

/* #######  Vector ####### */
/* Load/Store for vector */
void LoadVecReg(unsigned int vd_idx, int element, unsigned int rn_idx, int size);
void StoreVecReg(unsigned int rd_idx, int element, unsigned int vn_idx, int size);

/* AND/OR/EOR/BIC/NOT ... between vector registers */
void AndVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx);
void OrrVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx);
void EorVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx);
void BicVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx);
void NotVecReg(unsigned int rd_idx, unsigned int rm_idx);

/* Read Vector register to FP register */
void ReadVecReg(unsigned int fd_idx, unsigned int vn_idx, unsigned int index, int size);
/* Read Vector register to general register */
void ReadVecElem(unsigned int rd_idx, unsigned int vn_idx, unsigned int index, int size);
/* Write general register value tot Vector register */
void WriteVecElem(unsigned int vd_idx, unsigned int rn_idx, unsigned int index, int size);

/* Duplicate an immediate value to vector register */
void DupVecImmI32(unsigned int vd_idx, uint32_t imm, int size, int dstsize);
void DupVecImmI64(unsigned int vd_idx, uint64_t imm, int size, int dstsize);
/* Duplicate an element of vector register to new one */
void DupVecReg(unsigned int vd_idx, unsigned int vn_idx, unsigned int index, int size, int dstsize);
/* Duplicate an general register into vector register */
void DupVecRegFromGen(unsigned int vd_idx, unsigned int rn_idx, int size, int dstsize);

/* Compare Bit wise equal */
void CompareEqualVec(unsigned int vd_idx, unsigned int vn_idx, unsigned int vm_idx, int index, int size);

/* Compare Bit wise test bits nonzero */
void CompareTestBitsVec(unsigned int vd_idx, unsigned int vn_idx, unsigned int vm_idx, int index, int size);

};

/* Global Interpreter singleton class .*/
class Interpreter {
private:
Interpreter() = default;
~Interpreter() = default;

static Interpreter *inst;
static IntprCallback *disas_cb;
public:
Interpreter(const Interpreter&) = delete;
Interpreter& operator=(const Interpreter&) = delete;
Interpreter(Interpreter&&) = delete;
Interpreter& operator=(Interpreter&&) = delete;

void Init();

static Interpreter *get_instance() {
	return inst;
}

static void create() {
	if (!inst) {
		inst = new Interpreter;
		inst->disas_cb = new IntprCallback;
	}
}

static void destroy() {
	if (inst) {
		delete inst->disas_cb;
		delete inst;
		inst = nullptr;
	}
}
void Run();
int SingleStep();
};
#endif
