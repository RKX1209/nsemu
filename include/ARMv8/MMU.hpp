#ifndef _MMU_HPP
#define _MMU_HPP

namespace ARMv8 {

uint32_t ReadInst(uint64_t vaddr);
uint32_t ReadU32(const uint64_t paddr);

}
#endif
