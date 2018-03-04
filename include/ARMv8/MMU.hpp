#ifndef _MMU_HPP
#define _MMU_HPP

/* MMU interface that support access by guest virtual/physical address. */

namespace ARMv8 {

uint32_t ReadInst(uint64_t gva);
uint8_t ReadU8(const uint64_t gva);
uint16_t ReadU16(const uint64_t gva);
uint32_t ReadU32(const uint64_t gva);
uint64_t ReadU64(const uint64_t gva);

void WriteU8(const uint64_t gva, uint8_t value);
void WriteU16(const uint64_t gva, uint16_t value);
void WriteU32(const uint64_t gva, uint32_t value);
void WriteU64(const uint64_t gva, uint64_t value);

}
#endif
