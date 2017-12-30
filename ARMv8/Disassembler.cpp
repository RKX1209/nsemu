/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
namespace Disassembler {

static void UnallocatedOp(uint32_t insn) {
  ns_abort("Unallocated operation 0x%08lx\n", insn);
}

static void DisasPCRelAddr(uint32_t insn, DisasCallback *cb) {
  unsigned int rd, page;
  uint64_t offset, base;

  page = extract32(insn, 31, 1);
  offset = sextract64(insn, 5, 19);
  offset = offset << 2 | sextract64(insn, 29, 2);
  rd = extract32(insn, 0, 5);
  base = PC - 4;

  if (page) {
    base &= ~0xfff;
    offset <<= 12;
  }

  cb->MoviI64(rd, base + offset);
}

static void DisasAddSubImm(uint32_t insn, DisasCallback *cb) {
  unsigned int rd = extract32(insn, 0, 5);
  unsigned int rn = extract32(insn, 5, 5);
  uint64_t imm = extract32(insn, 10, 12);
  unsigned int shift = extract32(insn, 22, 2);
  bool setflags = extract32(insn, 29, 1);
  bool sub_op = extract32(insn, 30, 1);
  bool is_64bit = extract32(insn, 31, 1);

  switch (shift) {
   case 0x0:
       break;
   case 0x1:
       imm <<= 12;
       break;
   default:
       UnallocatedOp(insn);
       return;
  }

  if (sub_op) {
    cb->SubiI64(rd, rn, imm, setflags, is_64bit);
  } else {
    cb->AddiI64(rd, rn, imm, setflags, is_64bit);
  }
}

static void DisasLogImm(uint32_t insn, DisasCallback *cb) {
  unsigned long is_64bit = extract32(insn, 31, 1);
  unsigned long opc = extract32(insn, 29, 2);
  unsigned long is_n = extract32(insn, 22, 1);
  unsigned long immr = extract32(insn, 16, 6);
  unsigned long imms = extract32(insn, 10, 6);
  unsigned int rn = extract32(insn, 5, 5);
  unsigned int rd = extract32(insn, 0, 5);
  uint64_t wmask;

  // TODO: construct wmask

  switch (opc) {
    case 0x3: /* ANDS */
      cb->AndiI64(rd, rn, wmask, true, is_64bit);
      break;
    case 0x0: /* AND */
      cb->AndiI64(rd, rn, wmask, false, is_64bit);
      break;
    case 0x1: /* ORR */
      cb->OrrI64(rd, rn, wmask, is_64bit);
      break;
    case 0x2: /* EOR */
      cb->EorI64(rd, rn, wmask, is_64bit);
      break;
   default:
      ns_abort("Invalid Logical opcode: %u\n", opc);
   }
}

static void DisasDataProcImm(uint32_t insn, DisasCallback *cb) {
  switch (extract32(insn, 23, 6)){
    case 0x20: case 0x21: /* PC-rel. addressing */
      DisasPCRelAddr(insn, cb);
      break;
    case 0x22: case 0x23: /* Add/subtract (immediate) */
      DisasAddSubImm(insn, cb);
      break;
    case 0x24: /* Logical (immediate) */
      DisasLogImm(insn, cb);
      break;
    case 0x25: /* Move wide (immediate) */
      break;
    case 0x26: /* Bitfield */
      break;
    case 0x27: /* Extract */
      break;
    default:
      UnallocatedOp(insn);
      break;
  }
}

void DisasA64(uint32_t insn, DisasCallback *cb) {
  switch (extract32(insn, 25, 4)){
    case 0x0: case 0x1: case 0x2: case 0x3: //Unallocated
      UnallocatedOp(insn);
      break;
    case 0x8: case 0x9: /* Data processing - immediate */
      DisasDataProcImm(insn, cb);
      break;
    case 0xa: case 0xb: /* Branch, exception generation and system insns */
      break;
    case 0x4:
    case 0x6:
    case 0xc:
    case 0xe:      /* Loads and stores */
      break;
    case 0x5:
    case 0xd:      /* Data processing - register */
      break;
    case 0x7:
    case 0xf:      /* Data processing - SIMD and floating point */
      break;
    default:
      ns_abort("Invalid encoding operation: 0x%016lx\n", insn); /* all 15 cases should be handled above */
      break;
  }
}


};
