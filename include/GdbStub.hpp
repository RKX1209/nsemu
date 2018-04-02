#ifndef _GDBSTUB_HPP
#define _GDBSTUB_HPP

#define GDB_BREAKPOINT_SW        0
#define GDB_BREAKPOINT_HW        1
#define GDB_WATCHPOINT_WRITE     2
#define GDB_WATCHPOINT_READ      3
#define GDB_WATCHPOINT_ACCESS    4

#define GDB_BUFFER_SIZE 4096

#define DEBUG_REG_MAX   4

namespace GdbStub {

extern volatile bool enabled;
extern volatile bool step;

enum RSState {
    RS_INACTIVE,
    RS_IDLE,
    RS_GETLINE,
    RS_GETLINE_ESC,
    RS_GETLINE_RLE,
    RS_CHKSUM1,
    RS_CHKSUM2,
};

enum debug_reg {
	DEBUG_REG_DR0 = 0,
	DEBUG_REG_DR1 = 1,
	DEBUG_REG_DR2 = 2,
	DEBUG_REG_DR3 = 3,
	DEBUG_REG_DR7 = 7,
};

enum {
    GDB_SIGNAL_0 = 0,
    GDB_SIGNAL_INT = 2,
    GDB_SIGNAL_QUIT = 3,
    GDB_SIGNAL_TRAP = 5,
    GDB_SIGNAL_ABRT = 6,
    GDB_SIGNAL_ALRM = 14,
    GDB_SIGNAL_IO = 23,
    GDB_SIGNAL_XCPU = 24,
    GDB_SIGNAL_UNKNOWN = 143
};

class Breakpoint {
public:
        uint64_t addr;
        unsigned int len;
        int type;
        Breakpoint() { }
        Breakpoint(uint64_t a, unsigned int l, int t) : addr(a), len(l), type(t) { }
        bool operator < (const Breakpoint& bp) {
                if (addr == bp.addr) {
                        if (len == bp.len) {
                                return type < bp.type;
                        }
                        return len < bp.len;
                }
                return addr < bp.addr;
        }
        bool operator == (const Breakpoint& bp) {
                return addr == bp.addr && len == bp.len && type == bp.type;
        }
};

void Init();
void HandlePacket();
void Trap();

};
#endif
