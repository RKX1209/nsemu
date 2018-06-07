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
virtual void CondMovReg(unsigned int cond, unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool bit64) = 0;

/* Add/Sub with Immediate value */
virtual void AddI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) = 0;
virtual void SubI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) = 0;

/* Add/Sub/Mul/Div between registers */
virtual void AddReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void SubReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool setflags, bool bit64) = 0;
virtual void MulReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign, bool dst64, bool src64) = 0;
virtual void Mul2Reg(unsigned int rh_idx, unsigned int rl_idx, unsigned int rn_idx, unsigned int rm_idx, bool sign) = 0; //64bit * 64bit
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
virtual void LoadReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size, bool is_sign, bool extend, bool post, bool bit64) = 0;
virtual void LoadRegI64(unsigned int rd_idx, unsigned int ad_idx, int size, bool is_sign, bool extend) = 0;
virtual void StoreReg(unsigned int rd_idx, unsigned int base_idx, unsigned int rm_idx, int size, bool is_sign, bool extend, bool post, bool bit64) = 0;
virtual void StoreRegI64(unsigned int rd_idx, unsigned int ad_idx, int size, bool is_sign, bool extend) = 0;
virtual void _LoadReg(unsigned int rd_idx, uint64_t addr, int size, bool is_sign, bool extend) = 0;
virtual void _StoreReg(unsigned int rd_idx, uint64_t addr, int size, bool is_sign, bool extend) = 0;

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
/* Breakpoint exception */
virtual void BRK(unsigned int memo) = 0;

/* Read/Write Sysreg */
virtual void ReadWriteSysReg(unsigned int rd_idx, int offset, bool read) = 0;
/* Read/Write NZCV */
virtual void ReadWriteNZCV(unsigned int rd_idx, bool read) = 0;

/* Fp Mov between registers */
virtual void FMovReg(unsigned int fd_idx, unsigned int fn_idx, int type) = 0;

/* #######  Vector ####### */

/* AND/OR/EOR/BIC/NOT ... between vector registers */
virtual void AndVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx) = 0;
virtual void OrrVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx) = 0;
virtual void EorVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx) = 0;
virtual void BicVecReg(unsigned int rd_idx, unsigned int rn_idx, unsigned int rm_idx) = 0;
virtual void NotVecReg(unsigned int rd_idx, unsigned int rm_idx) = 0;

/* Load/Store for vector */
virtual void LoadVecReg(unsigned int vd_idx, int element, unsigned int rn_idx, int size) = 0;
virtual void StoreVecReg(unsigned int rd_idx, int element, unsigned int vn_idx, int size) = 0;

/* Load/Store for FP */
virtual void LoadFpRegI64(unsigned int fd_idx, unsigned int ad_idx, int size) = 0;
virtual void StoreFpRegI64(unsigned int fd_idx, unsigned int ad_idx, int size) = 0;

/* Read Vector register to FP regsiter */
virtual void ReadVecReg(unsigned int fd_idx, unsigned int vn_idx, unsigned int index, int size) = 0;
/* Read Vector register to general register */
virtual void ReadVecElem(unsigned int rd_idx, unsigned int vn_idx, unsigned int index, int size) = 0;
/* Write general register value tot Vector register */
virtual void WriteVecElem(unsigned int vd_idx, unsigned int rn_idx, unsigned int index, int size) = 0;

/* Duplicate an immediate value to vector register */
virtual void DupVecImmI32(unsigned int vd_idx, uint32_t imm, int size, int dstsize) = 0;
virtual void DupVecImmI64(unsigned int vd_idx, uint64_t imm, int size, int dstsize) = 0;

/* Duplicate an element of vector register to new one */
virtual void DupVecReg(unsigned int vd_idx, unsigned int vn_idx, unsigned int index, int size, int dstsize) = 0;

/* Duplicate an general register into vector register */
virtual void DupVecRegFromGen(unsigned int vd_idx, unsigned int rn_idx, int size, int dstsize) = 0;

/* Compare Bit wise equal */
virtual void CompareEqualVec(unsigned int vd_idx, unsigned int vn_idx, unsigned int vm_idx, int index, int size) = 0;

/* Compare Bit wise test bits nonzero */
virtual void CompareTestBitsVec(unsigned int vd_idx, unsigned int vn_idx, unsigned int vm_idx, int index, int size) = 0;

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

#define CP_REG_ARM64                   0x6000000000000000ULL
#define CP_REG_ARM_COPROC_MASK         0x000000000FFF0000
#define CP_REG_ARM_COPROC_SHIFT        16
#define CP_REG_ARM64_SYSREG            (0x0013 << CP_REG_ARM_COPROC_SHIFT)
#define CP_REG_ARM64_SYSREG_OP0_MASK   0x000000000000c000
#define CP_REG_ARM64_SYSREG_OP0_SHIFT  14
#define CP_REG_ARM64_SYSREG_OP1_MASK   0x0000000000003800
#define CP_REG_ARM64_SYSREG_OP1_SHIFT  11
#define CP_REG_ARM64_SYSREG_CRN_MASK   0x0000000000000780
#define CP_REG_ARM64_SYSREG_CRN_SHIFT  7
#define CP_REG_ARM64_SYSREG_CRM_MASK   0x0000000000000078
#define CP_REG_ARM64_SYSREG_CRM_SHIFT  3
#define CP_REG_ARM64_SYSREG_OP2_MASK   0x0000000000000007
#define CP_REG_ARM64_SYSREG_OP2_SHIFT  0

#define CP_REG_ARM64_SYSREG_CP (CP_REG_ARM64_SYSREG >> CP_REG_ARM_COPROC_SHIFT)

#define CP_REG_AA64_SHIFT 28
#define CP_REG_AA64_MASK (1 << CP_REG_AA64_SHIFT)

#define CP_ANY 0xff

#define ARM_CP_SPECIAL           0x0001
#define ARM_CP_CONST             0x0002
#define ARM_CP_64BIT             0x0004
#define ARM_CP_SUPPRESS_TB_END   0x0008
#define ARM_CP_OVERRIDE          0x0010
#define ARM_CP_ALIAS             0x0020
#define ARM_CP_IO                0x0040
#define ARM_CP_NO_RAW            0x0080
#define ARM_CP_NOP               (ARM_CP_SPECIAL | 0x0100)
#define ARM_CP_WFI               (ARM_CP_SPECIAL | 0x0200)
#define ARM_CP_NZCV              (ARM_CP_SPECIAL | 0x0300)
#define ARM_CP_CURRENTEL         (ARM_CP_SPECIAL | 0x0400)
#define ARM_CP_DC_ZVA            (ARM_CP_SPECIAL | 0x0500)
#define ARM_LAST_SPECIAL         ARM_CP_DC_ZVA
#define ARM_CP_FPU               0x1000
#define ARM_CP_SVE               0x2000
#define ARM_CP_SENTINEL          0xffff
#define ARM_CP_FLAG_MASK         0x30ff

enum {
    ARM_CP_STATE_AA32 = 0,
    ARM_CP_STATE_AA64 = 1,
    ARM_CP_STATE_BOTH = 2,
};
enum {
    ARM_CP_SECSTATE_S =   (1 << 0), /* bit[0]: Secure state register */
    ARM_CP_SECSTATE_NS =  (1 << 1), /* bit[1]: Non-secure state register */
};
#define PL3_R 0x80
#define PL3_W 0x40
#define PL2_R (0x20 | PL3_R)
#define PL2_W (0x10 | PL3_W)
#define PL1_R (0x08 | PL2_R)
#define PL1_W (0x04 | PL2_W)
#define PL0_R (0x02 | PL1_R)
#define PL0_W (0x01 | PL1_W)

#define PL3_RW (PL3_R | PL3_W)
#define PL2_RW (PL2_R | PL2_W)
#define PL1_RW (PL1_R | PL1_W)
#define PL0_RW (PL0_R | PL0_W)

#define ENCODE_SYSTEM_REG(cp, crn, crm, op0, op1, op2) \
        (CP_REG_AA64_MASK |                                 \
        ((cp) << CP_REG_ARM_COPROC_SHIFT) |                \
        ((op0) << CP_REG_ARM64_SYSREG_OP0_SHIFT) |         \
        ((op1) << CP_REG_ARM64_SYSREG_OP1_SHIFT) |         \
        ((crn) << CP_REG_ARM64_SYSREG_CRN_SHIFT) |         \
        ((crm) << CP_REG_ARM64_SYSREG_CRM_SHIFT) |         \
        ((op2) << CP_REG_ARM64_SYSREG_OP2_SHIFT))

typedef void A64DecodeFn(uint32_t insn, DisasCallback *cb);

class A64SysRegInfo {
public:
        std::string name;
        uint8_t cp;
        uint8_t crn;
        uint8_t crm;
        uint8_t opc0;
        uint8_t opc1;
        uint8_t opc2;
        int access;
        int state;
        int type;
        int offset;
        A64SysRegInfo (std::string _name, int _state, uint8_t _cp, uint8_t _opc0, uint8_t _opc1, uint8_t _opc2,
                        uint8_t _crn, uint8_t _crm, int _o) : name(_name), state(_state), cp(_cp), crn(_crn),
                        crm(_crm), opc0(_opc0), opc1(_opc1), opc2(_opc2), offset(_o) {}
        A64SysRegInfo (std::string _name, int _state, uint8_t _opc0, uint8_t _opc1, uint8_t _opc2,
                        uint8_t _crn, uint8_t _crm, int _o) : name(_name), state(_state),
                        opc0(_opc0), opc1(_opc1), opc2(_opc2), crn(_crn), crm(_crm), offset(_o) {}
        A64SysRegInfo (int _type) : type(_type) {}
};

typedef struct AArch64DecodeTable {
        uint32_t pattern;
        uint32_t mask;
        A64DecodeFn *disas_fn;
} A64DecodeTable;

void DisasA64(uint32_t insn, DisasCallback *cb);

void Init();
};
#endif
