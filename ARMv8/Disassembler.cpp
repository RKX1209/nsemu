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
	base = PC - 4;

	if (page) {
		base &= ~0xfff;
		offset <<= 12;
	}

	cb->MoviI64 (rd, base + offset, false, true);
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
		cb->SubiI64 (rd, rn, imm, setflags, is_64bit);
	} else {
		cb->AddiI64 (rd, rn, imm, setflags, is_64bit);
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
		cb->AndiI64 (rd, rn, wmask, true, is_64bit);
		break;
	case 0x0:	/* AND */
		cb->AndiI64 (rd, rn, wmask, false, is_64bit);
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
                cb->MoviI64 (rd, imm, false, is_64bit);
                break;
        case 3: /* MOVK */
                cb->MoviI64 (rd, imm, true, is_64bit);
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
                       // TODO: mov rd, rm
                       //cb->MovI64(rd, rm, sf);
               } else if (rm == rn) { /* ROR */
                       // TODO:
                       //cb->RorI64(rd, rm, imm)
               } else {
                       // TODO:
                       //cb->ShriI64(rm, rm, imm)
                       //cb->ShliI64(rn, rn, imm)
                       //cb->OrrI64(rd, rm, rn);
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

void DisasA64(uint32_t insn, DisasCallback *cb) {
	switch (extract32 (insn, 25, 4)) {
	case 0x0: case 0x1: case 0x2: case 0x3:	// Unallocated
		UnallocatedOp (insn);
		break;
	case 0x8: case 0x9:	/* Data processing - immediate */
		DisasDataProcImm (insn, cb);
		break;
	case 0xa: case 0xb:	/* Branch, exception generation and system insns */
		break;
	case 0x4:
	case 0x6:
	case 0xc:
	case 0xe:	/* Loads and stores */
		break;
	case 0x5:
	case 0xd:	/* Data processing - register */
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