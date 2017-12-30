#ifndef _DISASSEMBLER_HPP
#define _DISASSEMBLER_HPP

class DisasCallback {
public:
  virtual void MoviI64(unsigned int reg_idx, uint64_t imm) = 0;
  /* Add/Sub with Immediate value */
  virtual void AddiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) = 0;
  virtual void SubiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64) = 0;
  /* AND/OR/EOR... with Immediate value */
  virtual void AndiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64) = 0;
  virtual void OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) = 0;
  virtual void EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64) = 0;    
};

namespace Disassembler {

void DisasA64(uint32_t insn, DisasCallback *cb);

};

#endif
