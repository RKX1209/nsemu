/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
namespace Disassembler {

static void UnallocatedOp(uint32_t insn) {
	ns_abort ("Unallocated operation 0x%08lx\n", insn);
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
	offset = offset << 2 | sextract64 (insn, 29, 2);
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
                cb->MoviI64 (rd, imm, is_64bit);
                break;
        case 3: /* MOVK */
                //TODO: deposit(keep destionation bit)
                cb->DepositiI64 (rd, pos, imm, is_64bit);
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
                /* Wd<s-r:0> = Wn<s:r> */
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
                 * Wd<32+s-r,32-r> = Wn<s:0>
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
                //TODO: deposit
         } else {
                /* SBFM or UBFM: We start with zero, and we haven't modified
                   any bits outside bitsize, therefore the zero-extension
                   below is unneeded.  */
                //TODO: ???
                //tcg_gen_deposit_z_i64(tcg_rd, tcg_tmp, pos, len);
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
                       // TODO:
                       //cb->RorI64(rd, rm, imm)
               } else {
                       // TODO:
                       //cb->ShriI64(rm, rm, imm)
                       //cb->ShliI64(rn, rn, imm)
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
                cb->MoviI64(GPR_LR, addr, true);
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
        cb->AndI64(rt, rt, (1ULL << bit_pos), false, true);
        cb->BranchCondiI64 (op ? CondType_NE : CondType_EQ, rt, 0, addr, true);
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
        case 0: /* BR */
        case 1: /* BLR */
        case 2: /* RET */
                cb->SetPCReg (rn);
                /* BLR also needs to load return address */
                if (opc == 1) {
                        cb->MovReg(GPR_LR, rn, true);
                }
                break;
        case 4: /* ERET */
                //TODO:
                break;
        case 5: /* DRPS */
                //TODO:
                UnallocatedOp (insn);
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
                        break;
                case 3: /* SMC */
                        //TODO:
                        break;
                default:
                        UnallocatedOp (insn);
                        break;
                }
        case 1: /* BRK */
                //TODO:
                break;
        case 2: /* HLT */
                //TODO:
                break;
        case 5: /* DCPS1,2,3 */
                //TODO:
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
        cb->ExtendReg (rm, rm, option, sf);
        cb->ShiftI64 (rm, rm, ShiftType_LSL, imm3, sf);
        if (sub_op) {
                cb->SubReg (rd, rn, rm, setflags, sf);
        } else {
                cb->AddReg (rd, rn, rm, setflags, sf);
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

static void DisasDataProcReg(uint32_t insn, DisasCallback *cb) {
        switch (extract32(insn, 24, 5)) {
        case 0x0a: /* Logical (shifted register) */
                DisasLogicReg(insn, cb);
                break;
        case 0x0b: /* Add/subtract */
                if (insn & (1 << 21)) { /* (extended register) */
                        DisasAddSubExtReg (insn, cb);
                } else {
                        DisasAddSubReg (insn, cb);
                }
                break;
        case 0x1b: /* Data-processing (3 source) */
                /* TODO */
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

                        } else {            /* (2 source) */

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
		break;
	case 0x5:
	case 0xd:	/* Data processing - register */
                DisasDataProcReg (insn, cb);
		break;
	case 0x7:
	case 0xf:	/* Data processing - SIMD and floating point */
		break;
	default:
		ns_abort ("Invalid encoding operation: 0x%016lx\n", insn);	/* all 15 cases should be handled above */
		break;
	}
}

};
