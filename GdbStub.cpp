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
        snprintf(buf, sizeof(buf), "T%02xthread:%02x;", GDB_SIGNAL_TRAP, 1);
        WritePacket(buf);
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
            //res = gdb_breakpoint_insert(addr, len, type);
        } else {
            //res = gdb_breakpoint_remove(addr, len, type);
        }
        if (res >= 0)
            WritePacket("OK");
        else
        WritePacket("E22");
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
