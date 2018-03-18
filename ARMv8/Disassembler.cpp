/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
namespace Disassembler {

static inline void UnsupportedOp (const char *op) {
        ns_abort ("[TODO] Unsupported op %s (Disas fail)\n", op);
}

static inline void UnallocatedOp(uint32_t insn) {
	ns_abort ("Unallocated operation 0x%08lx\n", insn);
}

static inline bool FpAccessCheck(uint32_t insn) {
        /* TODO: */
        return true;
}

/* Currently used only fo disassemble fo SIMD operations */
static inline A64DecodeFn *LookupDisasFn(const A64DecodeTable *table,
                                         uint32_t insn) {
        const A64DecodeTable *tptr = table;
        while (tptr->mask) {
                if ((insn & tptr->mask) == tptr->pattern) {
                        return tptr->disas_fn;
                }
                tptr++;
        }
        return NULL;
}

static bool LogicImmDecode(uint64_t *wmask, unsigned int immn, unsigned int imms, unsigned int immr) {
	assert (immn < 2 && imms < 64 && immr < 64);
        uint64_t mask;
        unsigned int e, levels, s, r;
        int len = 31 - clz32((immn << 6 | (~imms & 0x3f)));
        if (len < 1) {
                /* This is the immn == 0, imms == 0x11111x case */
                return false;
        }
        e = 1 << len;

        levels = e - 1;
        s = imms & levels;
        r = immr & levels;

        if (s == levels) {
                /* <length of run - 1> mustn't be all-ones. */
                return false;
        }

        /* Create the value of one element: s+1 set bits rotated
         * by r within the element (which is e bits wide)...
         */
        mask = bitmask64(s + 1);
        if (r) {
                mask = (mask >> r) | (mask << (e - r));
                mask &= bitmask64(e);
        }
        mask = replicate64 (mask, e);
        *wmask = mask;
        return true;
}

static void DisasPCRelAddr(uint32_t insn, DisasCallback *cb) {
	unsigned int rd, page;
	uint64_t offset, base;

	page = extract32 (insn, 31, 1);
	offset = sextract64 (insn, 5, 19);
	offset = offset << 2 | extract32 (insn, 29, 2);
	rd = extract32 (insn, 0, 5);
	//base = PC - 4;
        base = PC;
	if (page) {
		base &= ~0xfff;
		offset <<= 12;
	}
	cb->MoviI64 (rd, base + offset, true);
}

static void DisasAddSubImm(uint32_t insn, DisasCallback *cb) {
	unsigned int rd = extract32 (insn, 0, 5);
	unsigned int rn = extract32 (insn, 5, 5);
	uint64_t imm = extract32 (insn, 10, 12);
	unsigned int shift = extract32 (insn, 22, 2);
	bool setflags = extract32 (insn, 29, 1);
	bool sub_op = extract32 (insn, 30, 1);
	bool is_64bit = extract32 (insn, 31, 1);

	switch (shift) {
	case 0x0:
		break;
	case 0x1:
		imm <<= 12;
		break;
	default:
		UnallocatedOp (insn);
		return;
	}

	if (sub_op) {
		cb->SubI64 (rd, rn, imm, setflags, is_64bit);
	} else {
		cb->AddI64 (rd, rn, imm, setflags, is_64bit);
	}
}

static void DisasLogImm(uint32_t insn, DisasCallback *cb) {
	unsigned long is_64bit = extract32 (insn, 31, 1);
	unsigned long opc = extract32 (insn, 29, 2);
	unsigned long is_n = extract32 (insn, 22, 1);
	unsigned long immr = extract32 (insn, 16, 6);
	unsigned long imms = extract32 (insn, 10, 6);
	unsigned int rn = extract32 (insn, 5, 5);
	unsigned int rd = extract32 (insn, 0, 5);
	uint64_t wmask;

	if (!LogicImmDecode (&wmask, is_n, imms, immr)) {
		UnallocatedOp (insn);
		return;
	}
        if (!is_64bit) {
                wmask &= 0xffffffff;
        }
	switch (opc) {
	case 0x3:	/* ANDS */
		cb->AndI64 (rd, rn, wmask, true, is_64bit);
		break;
	case 0x0:	/* AND */
		cb->AndI64 (rd, rn, wmask, false, is_64bit);
		break;
	case 0x1:	/* ORR */
		cb->OrrI64 (rd, rn, wmask, is_64bit);
		break;
	case 0x2:	/* EOR */
		cb->EorI64 (rd, rn, wmask, is_64bit);
		break;
	default:
		ns_abort ("Invalid Logical opcode: %u\n", opc);
	}
}

static void DisasMovwImm(uint32_t insn, DisasCallback *cb) {
        unsigned int is_64bit = extract32(insn, 31, 1);
        unsigned int opc = extract32(insn, 29, 2);
        uint64_t imm = extract32(insn, 5, 16);
        unsigned int pos = extract32(insn, 21, 2) << 4;
        unsigned int rd = extract32(insn, 0, 5);

        if (!is_64bit && (pos >= 32)) {
                UnallocatedOp (insn);
                return;
        }
        switch (opc) {
        case 0: /* MOVN */
        case 2: /* MOVZ */
                imm <<= pos;
                if (opc == 0) {
                        imm = ~imm;
                }
                if (!is_64bit) {
                        imm &= 0xffffffffu;
                }
                cb->MoviI64 (rd, imm, is_64bit);
                break;
        case 3: /* MOVK */
                cb->DepositI64 (rd, imm, pos, 16, is_64bit);
                break;
        default:
            UnallocatedOp (insn);
            break;
        }
}

static void DisasBitfield(uint32_t insn, DisasCallback *cb) {
        unsigned int is_64bit = extract32(insn, 31, 1);
        unsigned int opc = extract32(insn, 29, 2);
        unsigned int n = extract32(insn, 22, 1);
        unsigned int ri = extract32(insn, 16, 6);
        unsigned int si = extract32(insn, 10, 6);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);
        unsigned int bitsize = is_64bit ? 64 : 32;
        unsigned int pos, len;
        if (is_64bit != n || ri >= bitsize || si >= bitsize || opc > 2) {
                UnallocatedOp(insn);
                return;
        }

        /* Recognize simple(r) extractions.  */
        if (si >= ri) {
                /* Xd<0:r> = Xn<(s-r+1):s> */
                len = (si - ri) + 1;
                if (opc == 0) { /* SBFM: ASR, SBFX, SXTB, SXTH, SXTW */
                        cb->SExtractI64(rd, rn, ri, len, is_64bit);
                        return;
                } else if (opc == 2) { /* UBFM: UBFX, LSR, UXTB, UXTH */
                        cb->UExtractI64(rd, rn, ri, len, is_64bit);
                        return;
                }
                pos = 0;
        } else {
                /* Handle the ri > si case with a deposit
                 * Xd<64-r:64-r+s> = Xn<0:s>
                 */
                len = si + 1;
                pos = (bitsize - ri) & (bitsize - 1);
         }

         if (opc == 0 && len < ri) {
                /* SBFM: sign extend the destination field from len to fill
                   the balance of the word.  Let the deposit below insert all
                   of those sign bits.  */
                cb->SExtractI64(rd, rn, 0, len, is_64bit);
                len = ri;
         }

         if (opc == 1) { /* BFM, BXFIL */
                cb->DepositReg (rd, rd, pos, len, is_64bit);
         } else {
                /* SBFM or UBFM: We start with zero, and we haven't modified
                   any bits outside bitsize, therefore the zero-extension
                   below is unneeded.  */
                cb->DepositZeroReg (rd, rd, pos, len, is_64bit);
         }
         return;
}

static void DisasExtract(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int n = extract32(insn, 22, 1);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int imm = extract32(insn, 10, 6);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);
        unsigned int op21 = extract32(insn, 29, 2);
        unsigned int op0 = extract32(insn, 21, 1);
        unsigned int bitsize = sf ? 64 : 32;

        if (sf != n || op21 || op0 || imm >= bitsize) {
                UnallocatedOp (insn);
        } else {
               if (imm == 0) {
                       cb->MovReg(rd, rm, sf);
               } else if (rm == rn) { /* ROR */
                       cb->ShiftI64 (rd, rm, ShiftType_ROR, imm, sf);
               } else {
                       cb->ShiftI64 (rm, rm, ShiftType_LSR, imm, sf);
                       cb->ShiftI64 (rn, rn, ShiftType_LSR, imm, sf);
                       cb->OrrReg(rd, rm, rn, sf);
               }
       }
       return;
}

static void DisasDataProcImm(uint32_t insn, DisasCallback *cb) {
	switch (extract32 (insn, 23, 6)) {
	case 0x20: case 0x21:	/* PC-rel. addressing */
		DisasPCRelAddr (insn, cb);
		break;
	case 0x22: case 0x23:	/* Add/subtract (immediate) */
		DisasAddSubImm (insn, cb);
		break;
	case 0x24:	/* Logical (immediate) */
		DisasLogImm (insn, cb);
		break;
	case 0x25:	/* Move wide (immediate) */
		DisasMovwImm (insn, cb);
		break;
	case 0x26:	/* Bitfield */
		DisasBitfield (insn, cb);
		break;
	case 0x27:	/* Extract */
                DisasExtract (insn, cb);
		break;
	default:
		UnallocatedOp (insn);
		break;
	}
}

static void DisasUncondBrImm(uint32_t insn, DisasCallback *cb) {
        uint64_t addr = PC + sextract32(insn, 0, 26) * 4 - 4;
        if (insn & (1U << 31)) {
                /* BL Branch with link */
                cb->MoviI64(GPR_LR, PC, true);
        }

        /* B Branch / BL Branch with link */
        cb->BranchI64(addr);
}

static void DisasCompBrImm(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int op = extract32(insn, 24, 1); /* 0: CBZ; 1: CBNZ */
        unsigned int rt = extract32(insn, 0, 5);
        uint64_t addr = PC + sextract32(insn, 5, 19) * 4 - 4;
        cb->BranchCondiI64 (op ? CondType_NE : CondType_EQ, rt, 0, addr, sf);
}

static void DisasTestBrImm(uint32_t insn, DisasCallback *cb) {
        unsigned int bit_pos = (extract32(insn, 31, 1) << 5) | extract32(insn, 19, 5);
        unsigned int op = extract32(insn, 24, 1); /* 0: TBZ; 1: TBNZ */
        unsigned int addr = PC + sextract32(insn, 5, 14) * 4 - 4;
        uint64_t rt = extract32(insn, 0, 5);
        /* XXX: Dummy register is used as temporaly register. */
        cb->AndI64(GPR_DUMMY, rt, (1ULL << bit_pos), false, true);
        cb->BranchCondiI64 (op ? CondType_NE : CondType_EQ, GPR_DUMMY, 0, addr, true);
}

static void DisasCondBrImm(uint32_t insn, DisasCallback *cb) {
        if ((insn & (1 << 4)) || (insn & (1 << 24))) {
                UnallocatedOp (insn);
                return;
        }
        unsigned int cond = extract32(insn, 0, 4);
        uint64_t addr = PC + sextract32(insn, 5, 19) * 4 - 4;
        if (cond < 0xe) {
                cb->BranchFlag (cond, addr);
        } else {
                /* Always: */
                cb->BranchI64 (addr);
        }
}

static void DisasUncondBrReg(uint32_t insn, DisasCallback *cb) {
        unsigned int opc = extract32(insn, 21, 4);
        unsigned int op2 = extract32(insn, 16, 5);
        unsigned int op3 = extract32(insn, 10, 6);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int op4 = extract32(insn, 0, 5);

        if (op4 != 0x0 || op3 != 0x0 || op2 != 0x1f) {
                UnallocatedOp (insn);
                return;
        }

        switch (opc) {
        case 1: /* BLR */
                cb->MovReg(GPR_LR, PC_IDX, true);
        case 0: /* BR */
        case 2: /* RET */
                cb->SetPCReg (rn);
                break;
        case 4: /* ERET */
                //TODO:
                UnsupportedOp ("ERET");
                break;
        case 5: /* DRPS */
                //TODO:
                UnsupportedOp ("DRPS");
                break;
        default:
                UnallocatedOp (insn);
                break;
        }
}

static void DisasException(uint32_t insn, DisasCallback *cb) {
        unsigned int opc = extract32(insn, 21, 3);
        unsigned int op2_ll = extract32(insn, 0, 5);
        unsigned int imm16 = extract32(insn, 5, 16);
        switch (opc) {
        case 0:
                switch (op2_ll) {
                case 1: /* SVC */
                        cb->SVC (imm16);
                        break;
                case 2: /* HVC */
                        //TODO:
                        UnsupportedOp ("HVC");
                        break;
                case 3: /* SMC */
                        //TODO:
                        UnsupportedOp ("SMC");
                        break;
                default:
                        UnallocatedOp (insn);
                        break;
                }
                break;
        case 1: /* BRK */
                //TODO:
                UnsupportedOp ("BRK");
                break;
        case 2: /* HLT */
                UnsupportedOp ("HLT");
                //TODO:
                break;
        case 5: /* DCPS1,2,3 */
                //TODO:
                UnsupportedOp ("DCPS");
                break;
        default:
                UnallocatedOp (insn);
                break;

        }
}

static void DisasBranchExcSys(uint32_t insn, DisasCallback *cb) {
        switch (extract32(insn, 25, 7)) {
        case 0x0a: case 0x0b:
        case 0x4a: case 0x4b: /* Unconditional branch (immediate) */
                DisasUncondBrImm (insn, cb);
                break;
        case 0x1a: case 0x5a: /* Compare & branch (immediate) */
                DisasCompBrImm (insn, cb);
                break;
        case 0x1b: case 0x5b: /* Test & branch (immediate) */
                DisasTestBrImm (insn, cb);
                break;
        case 0x2a: /* Conditional branch (immediate) */
                DisasCondBrImm (insn, cb);
                break;
        case 0x6a: /* Exception generation / System */
                if (insn & (1 << 24)) {
                        //TODO:
                        //DisasSystem (insn, cb);
                        UnsupportedOp ("System");
                } else {
                        DisasException (insn, cb);
                }
                break;
        case 0x6b: /* Unconditional branch (register) */
                DisasUncondBrReg (insn, cb);
                break;
        default:
                UnallocatedOp (insn);
                break;
    }
}

static void DisasLogicReg(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int opc = extract32(insn, 29, 2);
        unsigned int shift_type = extract32(insn, 22, 2);
        unsigned int invert = extract32(insn, 21, 1);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int shift_amount = extract32(insn, 10, 6);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);
        if (!sf && (shift_amount & (1 << 5))) {
                UnallocatedOp (insn);
                return;
        }
        if (opc == 1 && shift_amount == 0 && shift_type == 0 && rn == 31) {
                /* Unshifted ORR and ORN with WZR/XZR is the standard encoding for
                * register-register MOV and MVN, so it is worth special casing.
                */
                if (invert) {
                        cb->NotReg (rd, rm, sf);
                }
                cb->MovReg (rd, rm, sf);
        }
        if (shift_amount) {
                cb->ShiftI64 (rm, rm, shift_type, shift_amount, sf);
        }
        switch (opc | (invert << 2)) {
        case 0: /* AND */
                cb->AndReg (rd, rn, rm, false, sf);
                break;
        case 3: /* ANDS */
                cb->AndReg (rd, rn, rm, true, sf);
                break;
        case 1: /* ORR */
                cb->OrrReg (rd, rn, rm, sf);
                break;
        case 2: /* EOR */
                cb->EorReg (rd, rn, rm, sf);
                break;
        case 4: /* BIC */
                cb->BicReg (rd, rn, rm, false, sf);
                break;
        case 7: /* BICS */
                cb->BicReg (rd, rn, rm, true, sf);
                break;
        case 5: /* ORN */
                cb->NotReg (rm, rm, sf);
                cb->OrrReg (rd, rn, rm, sf);
                break;
        case 6: /* EON */
                cb->NotReg (rm, rm, sf);
                cb->EorReg (rd, rn, rm, sf);
                break;
        default:
                ns_abort ("Invalid Logical(Reg) opcode: %u\n", opc);
        break;
    }
}

static void DisasAddSubExtReg(uint32_t insn, DisasCallback *cb) {
        unsigned int rd = extract32(insn, 0, 5);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int imm3 = extract32(insn, 10, 3);
        unsigned int option = extract32(insn, 13, 3);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned setflags = extract32(insn, 29, 1);
        unsigned sub_op = extract32(insn, 30, 1);
        unsigned sf = extract32(insn, 31, 1);
        if (imm3 > 4) {
                UnallocatedOp (insn);
                return;
        }
        cb->ExtendReg (GPR_DUMMY, rm, option, sf);
        cb->ShiftI64 (GPR_DUMMY, GPR_DUMMY, ShiftType_LSL, imm3, sf);
        if (sub_op) {
                cb->SubReg (rd, rn, GPR_DUMMY, setflags, sf);
        } else {
                cb->AddReg (rd, rn, GPR_DUMMY, setflags, sf);
        }
}

static void DisasAddSubReg(uint32_t insn, DisasCallback *cb) {
        unsigned int rd = extract32(insn, 0, 5);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int imm6 = extract32(insn, 10, 6);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int shift_type = extract32(insn, 22, 2);
        unsigned setflags = extract32(insn, 29, 1);
        unsigned sub_op = extract32(insn, 30, 1);
        unsigned sf = extract32(insn, 31, 1);
        if (extract32(insn, 10, 6) != 0) {
                UnallocatedOp (insn);
                return;
        }
        if (sub_op) {
                cb->SubReg (rd, rn, rm, setflags, sf);
        } else {
                cb->AddReg (rd, rn, rm, setflags, sf);
        }
}

static void DisasDataProc3src(uint32_t insn, DisasCallback *cb) {
        unsigned int rd = extract32(insn, 0, 5);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int ra = extract32(insn, 10, 5);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int op_id = (extract32(insn, 29, 3) << 4) |
                        (extract32(insn, 21, 3) << 1) | extract32(insn, 15, 1);
        bool sf = extract32(insn, 31, 1);
        bool is_sub = extract32(op_id, 0, 1);
        bool is_high = extract32(op_id, 2, 1);
        bool is_signed = false;
        bool src64 = false;

        /* Note that op_id is sf:op54:op31:o0 so it includes the 32/64 size flag */
        switch (op_id) {
        case 0x42: /* SMADDL */
        case 0x43: /* SMSUBL */
        case 0x44: /* SMULH */
                is_signed = true;
                break;
        case 0x0: /* MADD (32bit) */
        case 0x1: /* MSUB (32bit) */
        case 0x40: /* MADD (64bit) */
        case 0x41: /* MSUB (64bit) */
        case 0x4a: /* UMADDL */
        case 0x4b: /* UMSUBL */
        case 0x4c: /* UMULH */
                break;
        default:
                UnallocatedOp (insn);
                return;
        }
        if (is_high) {
                /* SMULH, UMULH ... [Multiply High] */
                cb->Mul2Reg (rd, GPR_DUMMY, rn, rm, is_signed);
        } else {
                /*
                 * sf = 0: MADD(32bit), MSUB(32bit) ... Wd = Wa +/- Wn * Wm
                 * sf = 1: (op_id < 0x42) MADD(64bit), MSUB(64bit) ... Xd = Xa +/- Xn * Xm
                           (sign = 0) UMADDL(UMULL), UMSUBL(UMNEGL) ... [Multiply Long 32bit*32bit] Xd = Wa +/- Wn * Wm
                 *         (sign = 1) SMADDL(SMULL), SMSUBL(SMNEGL) ... [Multiply Long 32bit*32bit] Xd = Wa +/- Wn * Wm
                 */
                if (op_id < 0x42) {
                        /* MADD(64bit), MSUB(64bit) */
                        src64 = true;
                }
                cb->MulReg (rd, rn, rm, is_signed, sf, src64);
                if (is_sub)
                        cb->SubReg (rd, ra, rd, false, src64);
                else
                        cb->AddReg (rd, ra, rd, false, src64);
        }

}

static void DisasAddSubcReg(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int sub_op = extract32(insn, 30, 1);
        unsigned int setflags = extract32(insn, 29, 1);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);
        if (sub_op) {
                /* ADC, ADCS */
                cb->SubcReg (rd, rn, rm, setflags, sf);
        } else {
                /* SBC, SBCS */
                cb->AddcReg (rd, rn, rm, setflags, sf);
        }
}

static void DisasCondCmp(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int op = extract32(insn, 30, 1);
        unsigned int is_imm = extract32(insn, 11, 1);
        unsigned int y = extract32(insn, 16, 5); /* y = rm (reg) or imm5 (imm) */
        unsigned int cond = extract32(insn, 12, 4);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int nzcv = extract32(insn, 0, 4);
        if (!extract32(insn, 29, 1)) {
                UnallocatedOp (insn);
                return;
        }
        if (insn & (1 << 10 | 1 << 4)) {
                UnallocatedOp (insn);
                return;
        }
        /* CCMN, CCMP */
        if (is_imm)
                cb->CondCmpI64(rn, y, nzcv, cond, op, sf);
        else
                cb->CondCmpReg(rn, y, nzcv, cond, op, sf);
}

static void DisasCondSel(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int else_inv = extract32(insn, 30, 1);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int cond = extract32(insn, 12, 4);
        unsigned int else_inc = extract32(insn, 10, 1);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);
        if (extract32(insn, 29, 1) || extract32(insn, 11, 1)) {
                /* S == 1 or op2<1> == 1 */
                UnallocatedOp (insn);
                return;
        }
        bool cond_inv = false;
        if (rn == 31 && rm == 31) {
                /* CSET  (CSINC <Wd>, WZR, WZR, invert(<cond>)) *
                 * CSETM (CSINV <Wd>, WZR, WZR, invert(<cond>)) */
                cond = cond ^ 1; // i.e. invert(<cond>)
        }
        if (else_inv)
                cb->NotReg (rm, rm, sf);
        if (else_inc)
                cb->AddI64 (rm, rm, 1, false, sf);
        cb->CondMovReg (cond, rd, rn, rm);
}

static void DisasDataProc1src(uint32_t insn, DisasCallback *cb) {
        if (extract32(insn, 29, 1) || extract32(insn, 16, 5)) {
                UnallocatedOp (insn);
                return;
        }
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int opcode = extract32(insn, 10, 6);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);

        switch (opcode) {
        case 0: /* RBIT */
                cb->RevBit (rd, rn, sf);
                break;
        case 1: /* REV16 */
                cb->RevByte16 (rd, rn, sf);
                break;
        case 2: /* REV32 */
                cb->RevByte32 (rd, rn, sf);
                break;
        case 3: /* REV64 */
                cb->RevByte64 (rd, rn, sf);
                break;
        case 4: /* CLZ */
                cb->CntLeadZero (rd, rn, sf);
                break;
        case 5: /* CLS */
                cb->CntLeadSign (rd, rn, sf);
                break;
        }
}

static void DisasDataProc2src(uint32_t insn, DisasCallback *cb) {
        unsigned int sf = extract32(insn, 31, 1);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int opcode = extract32(insn, 10, 6);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int rd = extract32(insn, 0, 5);
        if (extract32(insn, 29, 1)) {
                UnallocatedOp (insn);
                return;
        }
        switch (opcode) {
        case 2: /* UDIV */
                cb->DivReg (rd, rn, rm, false, sf);
                break;
        case 3: /* SDIV */
                cb->DivReg (rd, rn, rm, true, sf);
                break;
        case 8: /* LSLV */
                cb->ShiftReg (rd, rn, rm, ShiftType_LSL, sf);
                break;
        case 9: /* LSRV */
                cb->ShiftReg (rd, rn, rm, ShiftType_LSR, sf);
                break;
        case 10: /* ASRV */
                cb->ShiftReg (rd, rn, rm, ShiftType_ASR, sf);
                break;
        case 11: /* RORV */
                cb->ShiftReg (rd, rn, rm, ShiftType_ROR, sf);
                break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23: /* CRC32 */
        {
                unsigned int sz = extract32(opcode, 0, 2);
                bool crc32c = extract32(opcode, 2, 1);
                /* TODO: */
                UnsupportedOp ("CRC32");
                break;
         }
         default:
                UnallocatedOp (insn);
                break;
        }
}

static void DisasDataProcReg(uint32_t insn, DisasCallback *cb) {
        switch (extract32(insn, 24, 5)) {
        case 0x0a: /* Logical (shifted register) */
                DisasLogicReg (insn, cb);
                break;
        case 0x0b: /* Add/subtract */
                if (insn & (1 << 21)) { /* (extended register) */
                        DisasAddSubExtReg (insn, cb);
                } else {
                        DisasAddSubReg (insn, cb);
                }
                break;
        case 0x1b: /* Data-processing (3 source) */
                DisasDataProc3src (insn, cb);
                break;
        case 0x1a:
                switch (extract32(insn, 21, 3)) {
                case 0x0: /* Add/subtract (with carry) */
                        DisasAddSubcReg (insn, cb);
                        break;
                case 0x2: /* Conditional compare */
                        DisasCondCmp (insn, cb);
                        break;
                case 0x4: /* Conditional select */
                        DisasCondSel (insn, cb);
                        break;
                case 0x6: /* Data-processing */
                        if (insn & (1 << 30)) { /* (1 source) */
                                DisasDataProc1src (insn, cb);
                        } else {            /* (2 source) */
                                DisasDataProc2src (insn, cb);
                        }
                        break;
                default:
                        UnallocatedOp (insn);
                        break;
                }
                break;
        default:
                UnallocatedOp (insn);
                break;
        }
}

/* Load register (literal) ... literal means PC-relative immediate value */
static void DisasLdLit(uint32_t insn, DisasCallback *cb) {
        unsigned int rt = extract32(insn, 0, 5);
        int64_t imm = sextract32(insn, 5, 19) << 2;
        bool is_vector = extract32(insn, 26, 1);
        unsigned int opc = extract32(insn, 30, 2);
        bool sf = opc != 0;
        bool is_signed = false;
        int size = 2;
        if (is_vector) {
                /* LDR (SIMD&FP) */
                size = opc + 2;
        } else {
                if (opc == 3) {
                        return;
                }
                size = 2 + extract32(opc, 0, 1);
                is_signed = extract32(opc, 1, 1);
        }
        cb->AddI64 (rt, PC_IDX, imm - 4, false, true);
        cb->LoadRegI64 (rt, rt, size, is_signed, false);
}

static bool DisasLdstCompute64bit(unsigned int size, bool is_signed, unsigned int opc) {
        unsigned int opc0 = extract32(opc, 0, 1);
        unsigned int regsize;

        if (is_signed) {
                regsize = opc0 ? 32 : 64;
        } else {
                regsize = size == 3 ? 64 : 32;
        }
        return regsize == 64;
}

/* Load/Store register ... register offset */
static void DisasLdstRegRoffset(uint32_t insn, DisasCallback *cb,
                                unsigned int opc,
                                unsigned int size,
                                unsigned int rt,
                                bool is_vector) {
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int shift = extract32(insn, 12, 1);
        unsigned int rm = extract32(insn, 16, 5);
        unsigned int opt = extract32(insn, 13, 3);
        bool is_signed = false;
        bool is_store = false;
        bool is_extended = false;

        if (extract32(opt, 1, 1) == 0) {
                UnallocatedOp (insn);
                return;
        }

        if (is_vector) {
                /* "LDR/STR [base, Xm/Wm] (SIMD&FP)") */
                size = (opc & 2) << 1;
                is_store = !extract32(opc, 0, 1);
        } else {
                if (size == 3 && opc == 2) {
                        /* PRFM - prefetch */
                        return;
                }
                if (opc == 3 && size > 1) {
                        UnallocatedOp (insn);
                        return;
                }
                is_store = (opc == 0);
                is_signed = extract32(opc, 1, 1);
                //is_extended = (size < 3) && extract32(opc, 0, 1);
                is_extended = (size < 3); //TODO: treat other case, size = 0, 1(8bit-> or 16bit->)
        }
        bool sf = DisasLdstCompute64bit (size, is_signed, opc);
        cb->ExtendReg (GPR_DUMMY, rm, opt, sf); //FIXME: When rm == GPR_ZERO, it should be handled as GPR_SP
        cb->ShiftI64 (GPR_DUMMY, GPR_DUMMY, ShiftType_LSL, shift ? size : 0, sf);
        if (is_store) {
                cb->StoreReg (rt, rn, GPR_DUMMY, size, is_signed, is_extended, false, sf);
        } else {
                cb->LoadReg (rt, rn, GPR_DUMMY, size, is_signed, is_extended, false, sf);
        }
}

/*
 * Load/store register (unscaled immediate)
 * Load/store immediate pre/post-indexed
 * Load/store register unprivileged
 */
static void DisasLdstRegImm9(uint32_t insn, DisasCallback *cb,
                                unsigned int opc,
                                unsigned int size,
                                unsigned int rt,
                                bool is_vector) {
        unsigned int rn = ARMv8::HandleAsSP (extract32(insn, 5, 5));
        uint64_t imm9 = sextract32(insn, 12, 9);
        unsigned int idx = extract32(insn, 10, 2);
        bool is_signed = false;
        bool is_store = false;
        bool is_extended = false;
        bool is_unpriv = (idx == 2);
        bool iss_valid = !is_vector;
        bool post_index;
        bool writeback;

        //debug_print ("ldst uimm9\n");

        if (is_vector) {
                /* LDR/STR [base, #imm9] (SIMD&FP) */
                size |= (opc & 2) << 1;
                if (size > 4 && is_unpriv) {
                        UnallocatedOp (insn);
                        return;
                }
                is_store = ((opc & 1) == 0);
        } else {
                if (size == 3 && opc == 2) {
                        /* PRFM - prefetch */
                        if (is_unpriv) {
                                UnallocatedOp (insn);
                                return;
                        }
                        return;
                }
                if (opc == 3 && size > 1) {
                        UnallocatedOp (insn);
                        return;
                }
                is_store = (opc == 0);
                is_signed = extract32(opc, 1, 1);
                //is_extended = (size < 3) && extract32(opc, 0, 1);
                is_extended = (size < 3); //TODO: treat other case, size = 0, 1(8bit-> or 16bit->)
        }
        bool sf = DisasLdstCompute64bit (size, is_signed, opc);
        switch (idx) {
        case 0:
        case 2:
                post_index = false;
                writeback = false;
                break;
        case 1:
                post_index = true;
                writeback = true;
                break;
        case 3:
                post_index = false;
                writeback = true;
                break;
        default:
                ns_abort ("Unreachable status\n");
        }

        cb->MovReg (GPR_DUMMY, rn, true);
        if (!post_index) {
                cb->AddI64 (GPR_DUMMY, rn, imm9, false, true);
        }
        if (is_store) {
                cb->StoreRegI64 (rt, GPR_DUMMY, size, is_signed, is_extended);
        } else {
                cb->LoadRegI64 (rt, GPR_DUMMY, size, is_signed, is_extended);
        }
        if (writeback) {
                cb->AddI64 (rn, rn, imm9, false, true);
        }
}

static void DisasLdstRegUnsignedImm(uint32_t insn, DisasCallback *cb,
                                unsigned int opc,
                                unsigned int size,
                                unsigned int rt,
                                bool is_vector) {
        unsigned int rn = ARMv8::HandleAsSP (extract32(insn, 5, 5));
        uint64_t imm12 = extract32(insn, 10, 12);
        uint64_t offset;

        bool is_store;
        bool is_signed = false;
        bool is_extended = false;
        //debug_print ("ldst unsigned imm\n");
        if (is_vector) {
                /* LDR/STR [base, #uimm12] (SIMD&FP) */
                size |= (opc & 2) << 1;
                if (size > 4) {
                        UnallocatedOp (insn);
                        return;
                }
                is_store = !extract32(opc, 0, 1);
                if (!FpAccessCheck (insn)) {
                        return;
                }
        } else {
                if (size == 3 && opc == 2) {
                        /* PRFM - prefetch */
                        return;
                }
                if (opc == 3 && size > 1) {
                        UnallocatedOp (insn);
                        return;
                }
                is_store = (opc == 0);
                is_signed = extract32(opc, 1, 1);
                //is_extended = (size < 3) && extract32(opc, 0, 1);
                is_extended = (size < 3); //TODO: treat other case, size = 0, 1(8bit-> or 16bit->)
        }
        offset = imm12 << size;
        cb->AddI64 (GPR_DUMMY, rn, offset, false, true);
        if (is_vector) {
                /* size must be 4 (128-bit) */
                if (is_store) {
                        cb->StoreRegI64 (rt, GPR_DUMMY, size, is_signed, false);
                } else {
                        cb->LoadRegI64 (rt, GPR_DUMMY, size, is_signed, false);
                }
        } else {
                bool sf = DisasLdstCompute64bit (size, is_signed, opc);
                if (is_store) {
                        cb->StoreRegI64 (rt, GPR_DUMMY, size, is_signed, is_extended);
                } else {
                        cb->LoadRegI64 (rt, GPR_DUMMY, size, is_signed, is_extended);
                }
        }
}

/* Load/Store register ... register offset mode */
static void DisasLdstReg(uint32_t insn, DisasCallback *cb) {
        unsigned int rt = extract32(insn, 0, 5);
        unsigned int opc = extract32(insn, 22, 2);
        bool is_vector = extract32(insn, 26, 1);
        unsigned int size = extract32(insn, 30, 2);

        switch (extract32(insn, 24, 2)) {
        case 0:
                if (extract32(insn, 21, 1) == 1 && extract32(insn, 10, 2) == 2) {
                        DisasLdstRegRoffset (insn, cb, opc, size, rt, is_vector);
                } else {
                        /*
                         * Load/store register (unscaled immediate)
                         * Load/store immediate pre/post-indexed
                         * Load/store register unprivileged
                         */
                        DisasLdstRegImm9 (insn, cb, opc, size, rt, is_vector);
                }
                break;
        case 1:
                DisasLdstRegUnsignedImm (insn, cb, opc, size, rt, is_vector);
                break;
        default:
                UnallocatedOp (insn);
                break;
        }
}

/*
 * Load/store register (unscaled immediate)
 * Load/store immediate pre/post-indexed
 * Load/store register unprivileged
 */
static void DisasLdstPair(uint32_t insn, DisasCallback *cb) {
        unsigned int rt = extract32(insn, 0, 5);
        unsigned int rn = ARMv8::HandleAsSP (extract32(insn, 5, 5));
        unsigned int rt2 = extract32(insn, 10, 5);
        uint64_t offset = sextract64(insn, 15, 7);
        unsigned int index = extract32(insn, 23, 2);
        bool is_vector = extract32(insn, 26, 1);
        bool is_load = extract32(insn, 22, 1);
        unsigned int opc = extract32(insn, 30, 2);

        bool is_signed = false;
        bool post_index = false;
        bool writeback = false;

        int size;

        if (opc == 3) {
                UnallocatedOp (insn);
                return;
        }

        if (is_vector) {
                /* LDP/STP Xt1, Xt2, [base, #simm7] (SIMD&FP) */
                size = 2 + opc;
        } else {
                size = 2 + extract32 (opc, 1, 1);
                is_signed = extract32 (opc, 0, 1);
                if (!is_load && is_signed) {
                        UnallocatedOp (insn);
                        return;
                }
        }

        bool sf = DisasLdstCompute64bit (size, is_signed, opc);

        switch (index) {
        case 0:
                if (is_signed) {
                        /* There is no non-temporal-hint version of LDPSW */
                        UnallocatedOp (insn);
                        return;
                }
                post_index = false;
                break;
        case 1: /* post-index */
                post_index = true;
                writeback = true;
                break;
        case 2: /* signed offset, rn not update */
                post_index = false;
                break;
        case 3: /* pre-index */
                post_index = false;
                writeback = true;
                break;
        default:
                ns_abort ("Unreachable status\n");
        }

        offset <<= size;
        cb->MovReg (GPR_DUMMY, rn, true);
        if (!post_index) {
                cb->AddI64 (GPR_DUMMY, rn, offset, false, true);
        }
        if (is_load) {
                /* XXX: Do not modify rt register before recognizing any exception
                 * from the second load. */
                cb->LoadRegI64 (rt, GPR_DUMMY, size, is_signed, false);
                cb->AddI64 (GPR_DUMMY, GPR_DUMMY, 1 << size, false, true);
                cb->LoadRegI64 (rt2, GPR_DUMMY, size, is_signed, false);
        } else {
                cb->StoreRegI64 (rt, GPR_DUMMY, size, is_signed, false);
                cb->AddI64 (GPR_DUMMY, GPR_DUMMY, 1 << size, false, true);
                cb->StoreRegI64 (rt2, GPR_DUMMY, size, is_signed, false);
        }
        if (writeback) {
                cb->AddI64 (rn, rn, offset, false, true);
        }
}

static void DisasLdSt(uint32_t insn, DisasCallback *cb) {
        switch (extract32(insn, 24, 6)) {
        case 0x08: /* Load/store exclusive */
                //DisasLdstExcl (insn, cb);
                UnsupportedOp("Load/Store Exclusive");
                break;
        case 0x18: case 0x1c: /* Load register (literal) */
                DisasLdLit (insn, cb);
                break;
        case 0x28: case 0x29:
        case 0x2c: case 0x2d: /* Load/store pair (all forms) */
                DisasLdstPair (insn, cb);
                break;
        case 0x38: case 0x39:
        case 0x3c: case 0x3d: /* Load/store register (all forms) */
                DisasLdstReg (insn, cb);
                break;
        case 0x0c: /* AdvSIMD load/store multiple structures */
                UnsupportedOp("SIMD Load/Store Multi");
                break;
        case 0x0d: /* AdvSIMD load/store single structure */
                UnsupportedOp("SIMD Load/Store Single");
                break;
        default:
                UnallocatedOp (insn);
                break;
        }
}

static void DisasFp1Src(uint32_t insn, DisasCallback *cb) {
        //TODO:
}

static void DisasDataProcFp(uint32_t insn, DisasCallback *cb) {
        if (extract32(insn, 24, 1)) {
                /* Floating point data-processing (3 source) */
                //DisasFp3Src (insn);
                UnsupportedOp ("FP 3 source");
        } else if (extract32(insn, 21, 1) == 0) {
                /* Floating point to fixed point conversions */
                //DisasFpFixedConv (insn);
                UnsupportedOp ("FP fixed conv");
        } else {
                switch (extract32(insn, 10, 2)) {
                case 1:
                        /* Floating point conditional compare */
                        //DisasFpCcomp (insn);
                        break;
                case 2:
                        /* Floating point data-processing (2 source) */
                        //DisasFp2Src (insn);
                        break;
                case 3:
                        /* Floating point conditional select */
                        //DisasFpCsel (insn);
                        break;
                case 0:
                        switch (ctz32(extract32(insn, 12, 4))) {
                        case 0: /* [15:12] == xxx1 */
                                /* Floating point immediate */
                                //DisasFpImm (insn);
                                break;
                        case 1: /* [15:12] == xx10 */
                                /* Floating point compare */
                                //DisasFpCompare (insn);
                                break;
                        case 2: /* [15:12] == x100 */
                                /* Floating point data-processing (1 source) */
                                DisasFp1Src (insn, cb);
                                break;
                        case 3: /* [15:12] == 1000 */
                                UnallocatedOp (insn);
                                break;
                        default: /* [15:12] == 0000 */
                                /* Floating point <-> integer conversions */
                                //disas_fp_int_conv(s, insn);
                                break;
                        }
                        break;
                }
        }
}

static void DisasSimdDupe(uint32_t insn, int is_q, int rd, int rn,
                          int imm5, DisasCallback *cb) {
        int size = ctz32(imm5);
        unsigned int index = imm5 >> (size + 1);

        if (size > 3 || (size == 3 && !is_q)) {
                UnallocatedOp (insn);
                return;
        }
        cb->DupVecReg(rd, rn, index, size, (imm5 >> 4) ? 128 : 64);
}

static void DisasSimdDupg(uint32_t insn, int is_q, int rd, int rn,
                          int imm5, DisasCallback *cb) {
        int size = ctz32(imm5);

        if (size > 3 || (size == 3 && !is_q)) {
                UnallocatedOp (insn);
                return;
        }
        cb->DupVecRegFromGen(rd, rn, size, is_q ? 128 : 64);
}

static void DisasSimdCopy(uint32_t insn, DisasCallback *cb) {
        unsigned int rd = extract32(insn, 0, 5);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int imm4 = extract32(insn, 11, 4);
        unsigned int op = extract32(insn, 29, 1);
        unsigned int is_q = extract32(insn, 30, 1);
        unsigned int imm5 = extract32(insn, 16, 5);

        if (op) {
                if (is_q) {
                        /* INS (element) */
                        //handle_simd_inse(s, rd, rn, imm4, imm5);
                } else {
                        UnallocatedOp (insn);
                }
        } else {
                switch (imm4) {
                case 0:
                        /* DUP (element - vector) */
                        DisasSimdDupe(insn, is_q, rd, rn, imm5, cb);
                        break;
                case 1:
                        /* DUP (general) */
                        DisasSimdDupg(insn, is_q, rd, rn, imm5, cb);
                        break;
                case 3:
                        if (is_q) {
                                /* INS (general) */
                                //handle_simd_insg(s, rd, rn, imm5);
                        } else {
                                UnallocatedOp (insn);
                        }
                        break;
                case 5:
                case 7:
                        /* UMOV/SMOV (is_q indicates 32/64; imm4 indicates signedness) */
                        //handle_simd_umov_smov(s, is_q, (imm4 == 5), rn, rd, imm5);
                        break;
                default:
                        UnallocatedOp (insn);
                        break;
                }
        }
}

static void DisasSimdScalarCopy(uint32_t insn, DisasCallback *cb) {
        unsigned int fd = extract32(insn, 0, 5);
        unsigned int rn = extract32(insn, 5, 5);
        unsigned int imm4 = extract32(insn, 11, 4);
        unsigned int imm5 = extract32(insn, 16, 5);
        unsigned int op = extract32(insn, 29, 1);
        int size = ctz32(imm5);

        if (op != 0 || imm4 != 0) {
                UnallocatedOp (insn);
                return;
        }
        if (size > 3) {
                UnallocatedOp (insn);
        }
        int index = imm5 >> (size + 1);
        cb->ReadVecReg (fd, rn, index, size);
}

/* C4.1.5 Data Processing -- Scalar Floating-Point and Advanced SIMD
 */
static const A64DecodeTable data_proc_simd[] = {
    /* pattern  ,  mask     ,  fn                        */
    // { 0x0e200400, 0x9f200400, disas_simd_three_reg_same },
    // { 0x0e008400, 0x9f208400, disas_simd_three_reg_same_extra },
    // { 0x0e200000, 0x9f200c00, disas_simd_three_reg_diff },
    // { 0x0e200800, 0x9f3e0c00, disas_simd_two_reg_misc },
    // { 0x0e300800, 0x9f3e0c00, disas_simd_across_lanes },
    { 0x0e000400, 0x9fe08400, DisasSimdCopy },
    // { 0x0f000000, 0x9f000400, disas_simd_indexed }, /* vector indexed */
    // /* simd_mod_imm decode is a subset of simd_shift_imm, so must precede it */
    // { 0x0f000400, 0x9ff80400, disas_simd_mod_imm },
    // { 0x0f000400, 0x9f800400, disas_simd_shift_imm },
    // { 0x0e000000, 0xbf208c00, disas_simd_tb },
    // { 0x0e000800, 0xbf208c00, disas_simd_zip_trn },
    // { 0x2e000000, 0xbf208400, disas_simd_ext },
    // { 0x5e200400, 0xdf200400, disas_simd_scalar_three_reg_same },
    // { 0x5e008400, 0xdf208400, disas_simd_scalar_three_reg_same_extra },
    // { 0x5e200000, 0xdf200c00, disas_simd_scalar_three_reg_diff },
    // { 0x5e200800, 0xdf3e0c00, disas_simd_scalar_two_reg_misc },
    // { 0x5e300800, 0xdf3e0c00, disas_simd_scalar_pairwise },
    { 0x5e000400, 0xdfe08400, DisasSimdScalarCopy },
    // { 0x5f000000, 0xdf000400, disas_simd_indexed }, /* scalar indexed */
    // { 0x5f000400, 0xdf800400, disas_simd_scalar_shift_imm },
    // { 0x4e280800, 0xff3e0c00, disas_crypto_aes },
    // { 0x5e000000, 0xff208c00, disas_crypto_three_reg_sha },
    // { 0x5e280800, 0xff3e0c00, disas_crypto_two_reg_sha },
    // { 0xce608000, 0xffe0b000, disas_crypto_three_reg_sha512 },
    // { 0xcec08000, 0xfffff000, disas_crypto_two_reg_sha512 },
    // { 0xce000000, 0xff808000, disas_crypto_four_reg },
    // { 0xce800000, 0xffe00000, disas_crypto_xar },
    // { 0xce408000, 0xffe0c000, disas_crypto_three_reg_imm2 },
    // { 0x0e400400, 0x9f60c400, disas_simd_three_reg_same_fp16 },
    // { 0x0e780800, 0x8f7e0c00, disas_simd_two_reg_misc_fp16 },
    // { 0x5e400400, 0xdf60c400, disas_simd_scalar_three_reg_same_fp16 },
    { 0x00000000, 0x00000000, NULL }
};

static void DisasDataProcSimd(uint32_t insn, DisasCallback *cb) {
        A64DecodeFn *fn = LookupDisasFn (&data_proc_simd[0], insn);
        if (fn) {
                fn (insn, cb);
        } else {
                UnallocatedOp (insn);
        }
}

static void DisasDataProcSimdFp(uint32_t insn, DisasCallback *cb) {
        if (extract32(insn, 28, 1) == 1 && extract32(insn, 30, 1) == 0) {
                UnsupportedOp ("FP ops");
                //DisasDataProcFp(s, insn);
        } else {
                /* SIMD, including crypto */
                DisasDataProcSimd(insn, cb);
        }
}

void DisasA64(uint32_t insn, DisasCallback *cb) {
	switch (extract32 (insn, 25, 4)) {
	case 0x0: case 0x1: case 0x2: case 0x3:	// Unallocated
		UnallocatedOp (insn);
		break;
	case 0x8: case 0x9:	/* Data processing - immediate */
		DisasDataProcImm (insn, cb);
		break;
	case 0xa: case 0xb:	/* Branch, exception generation and system insns */
		DisasBranchExcSys (insn, cb);
		break;
	case 0x4:
	case 0x6:
	case 0xc:
	case 0xe:	/* Loads and stores */
                DisasLdSt (insn, cb);
		break;
	case 0x5:
	case 0xd:	/* Data processing - register */
                DisasDataProcReg (insn, cb);
		break;
	case 0x7:
	case 0xf:	/* Data processing - SIMD and floating point */
                DisasDataProcSimdFp (insn, cb);
		break;
	default:
		ns_abort ("Invalid encoding operation: 0x%016lx\n", insn);	/* all 15 cases should be handled above */
		break;
	}
}

};
