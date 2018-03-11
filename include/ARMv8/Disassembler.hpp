#ifndef _DISASSEMBLER_HPP
#define _DISASSEMBLER_HPP

class DisasCallback {
public:
/* Mov with Immediate value */
virtual void MoviI64(unsigned int reg_idx, uint64_t imm, bool bit64) = 0;

/* Deposit (i.e. distination register won't be changed) with Immediate value */
virtual void DepositI64(unsigned int rd_idx, uint64_t imm, unsigned int pos, unsigned int len, bool bit64) = 0;
virtual void DepositReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) = 0;
virtual void DepositZeroI64(unsigned int rd_idx, uint64_t imm, unsigned int pos, unsigned int len, bool bit64) = 0;
virtual void DepositZeroReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) = 0;

/* Mov between registers */
virtual void MovReg(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;

/* Conditional mov between registers */
virtual void CondMovReg(unsigned int cond, unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;

/* Add/Sub with Immediate value */
virtual void AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) = 0;
virtual void SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) = 0;

/* Add/Sub/Div between registers */
virtual void AddReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void SubReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void DivReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign, bool bit64) = 0;
virtual void ShiftReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, unsigned int shift_type, bool bit64) = 0;

/* Add/Sub with carry flag between registers */
virtual void AddcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void SubcReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;

/* AND/OR/EOR/Shift ... with Immediate value */
virtual void AndI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64) = 0;
virtual void OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) = 0;
virtual void EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) = 0;
virtual void ShiftI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int shift_type, unsigned int shift_amount, bool bit64) = 0;

/* AND/OR/EOR/BIC/NOT... between registers */
virtual void AndReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void OrrReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) = 0;
virtual void EorReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) = 0;
virtual void BicReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void NotReg(unsigned int rd_idx, unsigned int rm_idx, bool bit64) = 0;
virtual void ExtendReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int extend_type, bool bit64) = 0;

/* Load/Store */
virtual void LoadReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size, bool extend, bool post, bool bit64) = 0;
virtual void LoadRegI64(unsigned int rd_idx, unsigned int base_idx, uint64_t offset, int size, bool extend, bool post) = 0;
virtual void StoreReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size, bool extend, bool post, bool bit64) = 0;
virtual void StoreRegI64(unsigned int rd_idx, unsigned int base_idx, uint64_t offset, int size, bool extend, bool post) = 0;

/* Bitfield Signed/Unsigned Extract... with Immediate value */
virtual void SExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) = 0;
virtual void UExtractI64(unsigned int rd_idx, unsigned int rn_idx, unsigned int pos, unsigned int len, bool bit64) = 0;

/* Reverse bit order */
virtual void RevBit(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;
/* Reverse byte order per 16bit */
virtual void RevByte16(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;
/* Reverse byte order per 32bit */
virtual void RevByte32(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;
/* Reverse byte order per 64bit */
virtual void RevByte64(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;
/* Count Leading Zeros */
virtual void CntLeadZero(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;
/* Count Leading Signed bits */
virtual void CntLeadSign(unsigned int rd_idx, unsigned int rn_idx, bool bit64) = 0;

/* Conditional compare... with Immediate value */
virtual void CondCmpI64(unsigned int rn_idx, unsigned int imm, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64) = 0;

/* Conditional compare... between registers */
virtual void CondCmpReg(unsigned int rn_idx, unsigned int rm_idx, unsigned int nzcv, unsigned int cond, unsigned int op, bool bit64) = 0;

/* Go to Immediate address */
virtual void BranchI64(uint64_t imm) = 0;

/* Conditional Branch with Immediate value and jump to Immediate address */
virtual void BranchCondiI64(unsigned int cond, unsigned int rt_idx, uint64_t imm, uint64_t addr, bool bit64) = 0;

/* Conditional Branch with NZCV flags */
virtual void BranchFlag(unsigned int cond, uint64_t addr) = 0;

/* Set PC with reg */
virtual void SetPCReg(unsigned int rt_idx) = 0;

/* Super Visor Call */
virtual void SVC(unsigned int svc_num) = 0;
};

namespace Disassembler {

//logical ops with shifted registers
enum ShiftType {
        ShiftType_LSL = 0,
        ShiftType_LSR,
        ShiftType_ASR,
        ShiftType_ROR
};

enum ExtendType {
        ExtendType_UXTB = 0,
        ExtendType_UXTH,
        ExtendType_UXTW,
        ExtendType_UXTX,
        ExtendType_SXTB,
        ExtendType_SXTH,
        ExtendType_SXTW,
        ExtendType_SXTX,
};

enum CondType {
        CondType_EQ = 0,
        CondType_NE,
        CondType_CSHS,
        CondType_CCLO,
        CondType_MI,
        CondType_PL,
        CondType_VS,
        CondType_VC,
        CondType_HI,
        CondType_LS,
        CondType_GE,
        CondType_LT,
        CondType_GT,
        CondType_LE,
        CondType_AL,
        CondType_NV
};

void DisasA64(uint32_t insn, DisasCallback *cb);

};
#endif
