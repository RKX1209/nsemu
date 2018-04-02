/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
/* TODO: Move host dependent functions to host_util.hpp */
#include "Nsemu.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace GdbStub {

static int server_fd, client_fd;

uint8_t cmd_buf[GDB_BUFFER_SIZE], last_packet[GDB_BUFFER_SIZE + 4];

static int buf_index, line_sum, checksum, last_packet_len;

static RSState state = RS_IDLE;

volatile bool enabled = false;
volatile bool step = false;

std::vector<Breakpoint> bp_list;

static inline bool IsXdigit(char ch) {
    return ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F') || ('0' <= ch && ch <= '9');
}

static inline int FromHex(int v)
{
    if (v >= '0' && v <= '9')
        return v - '0';
    else if (v >= 'A' && v <= 'F')
        return v - 'A' + 10;
    else if (v >= 'a' && v <= 'f')
        return v - 'a' + 10;
    else
        return 0;
}

static inline int ToHex(int v)
{
    if (v < 10)
        return v + '0';
    else
        return v - 10 + 'a';
}

static void MemToHex(char *buf, const uint8_t *mem, int len)
{
        char *q;
        q = buf;
        for(int i = 0; i < len; i++) {
                int c = mem[i];
                *q++ = ToHex(c >> 4);
                *q++ = ToHex(c & 0xf);
        }
        *q = '\0';
}

void Init() {
    struct sockaddr_in addr;

    server_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        ns_abort ("socket failed\n");
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ns_abort ("bind failed");
    }

    if (listen(server_fd, 5) != 0) {
    	ns_abort ("listen failed");
    }
    client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
        ns_abort ("Failed to accept gdb client\n");
    }
    enabled = true;
}

uint8_t ReadByte() {
    uint8_t byte;
    auto size = recv (client_fd, &byte, 1, MSG_WAITALL);
    if (size != 1) {
        ns_abort ("Failed recv :%ld", size);
    }
    return byte;
}

void WriteBytes(uint8_t *buf, size_t len) {
    if (send (client_fd, buf, len, 0) == -1) {
        ns_abort ("Failed send\n");
    }
}

void SendPacket(uint8_t *buf, size_t len) {
    int csum;
    uint8_t *p;
    for (;;) {
        p = last_packet;
        *(p++) = '$';
        memcpy(p, buf, len);
        p += len;
        csum = 0;
        for(int i = 0; i < len; i++) {
            csum += buf[i];
        }
        *(p++) = '#';
        *(p++) = ToHex((csum >> 4) & 0xf);
        *(p++) = ToHex((csum) & 0xf);

        last_packet_len = p - last_packet;
        WriteBytes (last_packet, last_packet_len);
        break;
  }
}

void WritePacket(std::string buf) {
    SendPacket ((uint8_t *)buf.c_str(), buf.size());
}

static int IsQueryPacket (const char *p, const char *query, char separator)
{
    unsigned int query_len = strlen(query);

    return strncmp(p, query, query_len) == 0 && (p[query_len] == '\0' || p[query_len] == separator);
}

static int TargetMemoryRW(uint64_t addr, uint8_t *buf, int len, bool is_write) {
        ns_print("[%s] addr 0x%016lx(%d byte)\n", (is_write?"WRITE":"READ"), addr, len);
        if ((int64_t)addr < 0) {
                return -1; //FIXME: Correct validation is required
        }
        if (is_write) {
                ARMv8::WriteBytes (addr, buf, len);
        } else {
                ARMv8::ReadBytes (addr, buf, len);
        }
        return 0;
}

static int ReadRegister (uint8_t *buf, int reg) {
        if (reg < GPR_ZERO) {
                *(uint64_t*)buf = X(reg);
        } else {
                switch(reg) {
                        case 31:
                                *(uint64_t *)buf = X(GPR_SP);
                                break;
                        case 32:
                                *(uint64_t *)buf = X(PC_IDX);
                                break;
                        default:
                                *(uint64_t *)buf = 0xdeadbeef;
                                break;
                }
        }
        return sizeof(uint64_t);
}

static void SendSignal(uint32_t signal) {
        char buf[GDB_BUFFER_SIZE];
        snprintf(buf, sizeof(buf), "T%02xthread:%02x;", signal, 1);
        WritePacket(buf);
}

void Trap() {
        SendSignal(GDB_SIGNAL_TRAP);
}

static void Stepi() {
        step = true;
}

static inline void AsmWridr0 (uint64_t dr0) {
	// asm volatile ("movq %0, %%dr0"
	// 	      :
	// 	      : "rm" ((uint64_t) dr0));
}
static inline void AsmWridr1 (unsigned long dr1) {
	// asm volatile ("movl %0, %%dr1"
	// 	      :
	// 	      : "rm" ((unsigned long) dr1));
}
static inline void AsmWridr2 (unsigned long dr2) {
	// asm volatile ("movl %0, %%dr2"
	// 	      :
	// 	      : "rm" ((unsigned long) dr2));
}
static inline void AsmWridr3 (unsigned long dr3) {
	// asm volatile ("movl %0, %%dr3"
	// 	      :
	// 	      : "rm" ((unsigned long) dr3));
}
static inline void AsmWridr7 (unsigned long dr7) {
	// asm volatile ("movl %0, %%dr7"
	// 	      :
	// 	      : "rm" ((unsigned long) dr7));
}

static inline void AsmRddr0 (unsigned long *dr0) {
	// asm volatile ("movl %%dr0,%0"
	// 	      : "=r" (*dr0));
}
static inline void AsmRddr1 (unsigned long *dr1) {
	// asm volatile ("movl %%dr1,%0"
	// 	      : "=r" (*dr1));
}
static inline void AsmRddr2 (unsigned long *dr2) {
	// asm volatile ("movl %%dr2,%0"
	// 	      : "=r" (*dr2));
}
static inline void AsmRddr3 (unsigned long *dr3) {
	// asm volatile ("movl %%dr3,%0"
	// 	      : "=r" (*dr3));
}
static inline void AsmRddr7 (unsigned long *dr7) {
	// asm volatile ("movl %%dr7,%0"
	// 	      : "=r" (*dr7));
}

void ReadDr(enum debug_reg reg, unsigned long *val)
{
	if (reg == DEBUG_REG_DR0)
		AsmRddr0 (val);
	else if (reg == DEBUG_REG_DR1)
		AsmRddr1 (val);
	else if (reg == DEBUG_REG_DR2)
		AsmRddr2 (val);
	else if (reg == DEBUG_REG_DR3)
		AsmRddr3 (val);
	else if (reg == DEBUG_REG_DR7)
		AsmRddr7 (val);
}

void WriteDr(enum debug_reg reg, unsigned long val)
{
	ns_print ("Write: dr%d = 0x%016lx\n", reg, val);
	if (reg == DEBUG_REG_DR0)
		AsmWridr0 (val);
	else if (reg == DEBUG_REG_DR1)
		AsmWridr1 (val);
	else if (reg == DEBUG_REG_DR2)
		AsmWridr2 (val);
	else if (reg == DEBUG_REG_DR3)
		AsmWridr3 (val);
	else if (reg == DEBUG_REG_DR7)
                AsmWridr7 (val);
}

static int SyncDebugReg() {
        std::map<int, uint8_t> type_code;
        type_code[GDB_BREAKPOINT_HW] = 0x0;
        type_code[GDB_WATCHPOINT_WRITE] = 0x1;
        type_code[GDB_WATCHPOINT_ACCESS] = 0x3;
        std::map<int, uint8_t> len_code;
        len_code[1] = 0x0;
        len_code[2] = 0x1;
        len_code[4] = 0x3;
        len_code[8] = 0x2;
        unsigned long dr7 = 0x0600;
        int n = 0;
        for (int n = 0; n < bp_list.size(); n++) {
                uint64_t addr = ARMv8::GvaToHva (bp_list[n].addr);
                ns_print("Set breakpoint at 0x%lx(0x%lx)\n", bp_list[n].addr, addr);
                WriteDr ((enum debug_reg) n, addr);
                dr7 |= (2 << (n * 2)) |
                (type_code[bp_list[n].type] << (16 + n * 4)) |
                ((uint32_t)len_code[bp_list[n].len] << (18 + n * 4));
        }
        WriteDr (DEBUG_REG_DR7, dr7);
        return 0;
}

static int HwBreakpointInsert(unsigned long addr, unsigned long len, int type) {
        if (bp_list.size() > DEBUG_REG_MAX) {
                return -1;
        }
        Breakpoint bp(addr, len, type);
        bp_list.push_back(bp);
        SyncDebugReg();
        return 0;
}

static int HwBreakpointRemove(unsigned long addr, unsigned long len, int type) {
        if (bp_list.empty()) {
                return -1;
        }
        Breakpoint bp(addr, len, type);
        bp_list.erase(std::remove(bp_list.begin(), bp_list.end(), bp), bp_list.end());
        SyncDebugReg();
        return 0;
}

static int BreakpointInsert(unsigned long addr, unsigned long len, int type) {
        int err = -1;
        switch (type) {
                case GDB_BREAKPOINT_SW:
                case GDB_BREAKPOINT_HW:
                        return HwBreakpointInsert (addr, len, type);
                case GDB_WATCHPOINT_WRITE:
                case GDB_WATCHPOINT_READ:
                case GDB_WATCHPOINT_ACCESS:
                /* TODO: */
                        break;
                default:
                        break;
        }
        return err;
}

static int BreakpointRemove(unsigned long addr, unsigned long len, int type) {
        int err = -1;
        switch (type) {
                case GDB_BREAKPOINT_SW:
                case GDB_BREAKPOINT_HW:
                        return HwBreakpointRemove (addr, len, type);
                case GDB_WATCHPOINT_WRITE:
                case GDB_WATCHPOINT_READ:
                case GDB_WATCHPOINT_ACCESS:
                /* TODO: */
                        break;
                default:
                        break;
        }
        return err;
}

static RSState HandleCommand(char *line_buf) {
    const char *p;
    uint32_t thread;
    int ch, reg_size, type, res;
    char buf[GDB_BUFFER_SIZE];
    uint8_t mem_buf[GDB_BUFFER_SIZE];
    unsigned long addr, len;

    ns_print("command='%s'\n", line_buf);

    p = line_buf;
    ch = *p++;
    switch(ch) {
    case '?':
        /* XXX: Is it correct to fix thread id to '1'? */
        SendSignal(GDB_SIGNAL_TRAP);
        break;
    case 'c':
        if (*p != '\0') {
            addr = strtol(p, (char **)&p, 16);
        }
        //Continue ();
        break;
    case 'g':
        len = 0;
        for (addr = 0; addr < 33; addr++) {
                reg_size = ReadRegister (mem_buf + len, addr);
                len += reg_size;
        }
        MemToHex(buf, mem_buf, len);
        WritePacket(buf);
        break;
    case 'm':
        addr = strtoul(p, (char **)&p, 16);
        if (*p == ',')
            p++;
        len = strtol(p, NULL, 16);

        /* memtohex() doubles the required space */
        if (len > GDB_BUFFER_SIZE / 2) {
            WritePacket ("E22");
            break;
        }

        if (TargetMemoryRW (addr, mem_buf, len, false) != 0) {
                WritePacket ("E14");
        } else {
                MemToHex(buf, mem_buf, len);
                WritePacket(buf);
        }
        break;
    case 'p':
        addr = strtol(p, (char **)&p, 16);
        reg_size = ReadRegister (mem_buf, addr);
        if (reg_size) {
                MemToHex(buf, mem_buf, reg_size);
                WritePacket(buf);
        } else {
                WritePacket("E14");
        }
        break;
    case 'q':
    case 'Q':
        if (IsQueryPacket(p, "Supported", ':')) {
            snprintf(buf, sizeof(buf), "PacketSize=%x", GDB_BUFFER_SIZE);
            WritePacket(buf);
            break;
        } else if (strcmp(p, "C") == 0) {
            WritePacket("QC1");
            break;
        }
        goto unknown_command;
    case 'z':
    case 'Z':
        type = strtol(p, (char **)&p, 16);
        if (*p == ',')
            p++;
        addr = strtol(p, (char **)&p, 16);
        if (*p == ',')
            p++;
        len = strtol(p, (char **)&p, 16);
        if (ch == 'Z') {
                res = BreakpointInsert(addr, len, type);
        } else {
                res = BreakpointRemove(addr, len, type);
        }
        if (res >= 0)
            WritePacket("OK");
        else
        WritePacket("E22");
        break;
    case 's':
        Stepi();
        break;
    case 'H':
        type = *p++;
        thread = strtol(p, (char **)&p, 16);
        if (thread == -1 || thread == 0) {
            WritePacket("OK");
            break;
        }
        switch (type) {
        case 'c':
            WritePacket("OK");
            break;
        case 'g':
            WritePacket("OK");
            break;
        default:
            WritePacket("E22");
            break;
        }
        break;
    case 'T':
        thread = strtol(p, (char **)&p, 16);
        WritePacket("OK");
        break;
    default:
      unknown_command:
        /* put empty packet */
        buf[0] = '\0';
        WritePacket(buf);
        break;
    }
    return RS_IDLE;
}

void HandlePacket() {
    uint8_t ch = ReadByte();
    uint8_t reply;
    switch (state) {
        case RS_IDLE:
            if (ch == '$') {
                /* start of command packet */
                buf_index = 0;
                line_sum = 0;
                state = RS_GETLINE;
            } else {
            //printf("%s: received garbage between packets: 0x%x\n", __func__, ch);
            }
            break;
        case RS_GETLINE:
            if (ch == '}') {
                state = RS_GETLINE_ESC;
                line_sum += ch;
            } else if (ch == '*') {
                /* start run length encoding sequence */
                state = RS_GETLINE_RLE;
                line_sum += ch;
            } else if (ch == '#') {
                /* end of command, start of checksum*/
                state = RS_CHKSUM1;
            } else if (buf_index >= sizeof(cmd_buf) - 1) {
                state = RS_IDLE;
                ns_print ("Gdb: command buffer overrun, dropping command\n");
            } else {
                /* unescaped command character */
                cmd_buf[buf_index++] = ch;
                line_sum += ch;
            }
            break;
        case RS_GETLINE_ESC:
            if (ch == '#') {
                /* unexpected end of command in escape sequence */
                state = RS_CHKSUM1;
            } else if (buf_index >= sizeof(cmd_buf) - 1) {
                /* command buffer overrun */
                state = RS_IDLE;
                ns_print ("Gdb: command buffer overrun, dropping command\n");
            } else {
                /* parse escaped character and leave escape state */
                cmd_buf[buf_index++] = ch ^ 0x20;
                line_sum += ch;
                state = RS_GETLINE;
            }
            break;
        case RS_GETLINE_RLE:
            if (ch < ' ') {
                /* invalid RLE count encoding */
                state = RS_GETLINE;
                ns_print ("Gdb: got invalid RLE count: 0x%x\n", ch);
            } else {
                /* decode repeat length */
                int repeat = (unsigned char)ch - ' ' + 3;
                if (buf_index + repeat >= sizeof(cmd_buf) - 1) {
                    /* that many repeats would overrun the command buffer */
                    ns_print ("Gdb: command buffer overrun, dropping command\n");
                    state = RS_IDLE;
                } else if (buf_index < 1) {
                    /* got a repeat but we have nothing to repeat */
                    ns_print ("Gdb: got invalid RLE sequence\n");
                    state = RS_GETLINE;
                } else {
                    /* repeat the last character */
                    memset(cmd_buf + buf_index,
                          cmd_buf[buf_index - 1], repeat);
                    buf_index += repeat;
                    line_sum += ch;
                    state = RS_GETLINE;
                }
            }
            break;
        case RS_CHKSUM1:
            /* get high hex digit of checksum */
            if (!IsXdigit(ch)) {
                ns_print("Gdb: got invalid command checksum digit\n");
                state = RS_GETLINE;
                break;
            }
            cmd_buf[buf_index] = '\0';
            checksum = FromHex(ch) << 4;
            state = RS_CHKSUM2;
            break;
        case RS_CHKSUM2:
            /* get low hex digit of checksum */
            if (!IsXdigit(ch)) {
                ns_print("Gdb: got invalid command checksum digit\n");
                state = RS_GETLINE;
                break;
            }
            checksum |= FromHex(ch);

            if (checksum != (line_sum & 0xff)) {
                ns_print("Gdb: got command packet with incorrect checksum\n");
                /* send NAK reply */
                reply = '-';
                WriteBytes (&reply, 1);
                state = RS_IDLE;
            } else {
                ns_print("Gdb: valid packet!\n");
                /* send ACK reply */
                reply = '+';
                WriteBytes (&reply, 1);
                state = HandleCommand ((char *)cmd_buf);
            }
            break;
        default:
            ns_print("Gdb: got unknown status\n");
            break;
    }
}

};
