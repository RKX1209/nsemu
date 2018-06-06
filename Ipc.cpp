/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#define DEFINE_STUBS
#include "Nsemu.hpp"
#include "IpcStubs.hpp"

void IpcMessage::ParseMessage() {
        uint32_t *buf = (uint32_t *) raw_ptr;
        type = buf[0] & 0xFFFF;
        x_cnt = (buf[0] >> 16) & 0xF;
        a_cnt = (buf[0] >> 20) & 0xF;
	b_cnt = (buf[0] >> 24) & 0xF;
	w_cnt = buf[1] & 0x3FF;
	has_c = ((buf[1] >> 10) & 0x3) != 0;
	// domainHandle = 0;
	// domainCommand = 0;
	bool has_hd = (buf[1] >> 31) == 1; //Handle descriptor enabled?
	int pos = 2;
    if (has_hd) {
        uint32_t hd = buf[pos++];
        has_pid = hd & 1;
        copy_cnt = (hd >> 1) & 0xf;
        move_cnt = hd >> 5;
        if(has_pid) {
			pid = *((uint64_t *) &buf[pos]);
			pos += 2;
		}
		copy_off = pos * 4;
		pos += copy_cnt;
		move_off = pos * 4;
		pos += move_cnt;
    }
    desc_off = pos * 4;
    pos += x_cnt * 2;
	pos += a_cnt * 3;
	pos += b_cnt * 3;
    raw_off = pos * 4;
    if(pos & 3)
		pos += 4 - (pos & 3);
    if (is_domainobj && type == 4) {
        domain_cmd = buf[pos] & 0xff;
        domain_handle = buf[pos + 1];
        pos += 4;
    }
    payload_off = pos * 4;
    cmd_id = GetData<uint32_t>(0);
}

void IpcMessage::GenBuf(unsigned int _move_cnt, unsigned int _copy_cnt, unsigned int data_bytes) {
        move_cnt = _move_cnt;
        copy_cnt = _copy_cnt;
        uint8_t *obuf = raw_ptr;
        obuf[0] = 0;
        if(move_cnt != 0 || copy_cnt != 0) {
                obuf[1] = ((move_cnt != 0 && !is_domainobj) || copy_cnt != 0) ? (1U << 31) : 0;
        	obuf[2] = (copy_cnt << 1) | ((is_domainobj ? 0 : move_cnt) << 5);
        }

        auto pos = 2 + (((move_cnt != 0 && !is_domainobj) || copy_cnt != 0) ? (1 + move_cnt + copy_cnt) : 0);
        auto start = pos;
        if(pos & 3)
                pos += 4 - (pos & 3);
        if(is_domainobj) {
                obuf[pos] = move_cnt;
        	pos += 4;
        }
        realdata_off = is_domainobj ? move_cnt << 2 : 0;
        auto data_words = (realdata_off >> 2) + (data_bytes & 3) ? (data_bytes >> 2) + 1 : (data_bytes >> 2);

        obuf[1] |= 4 + (is_domainobj ? 4 : 0) + 4 + data_words;

        payload_off = pos * 4;
        obuf[pos] = byte_swap32_str("SFCO");
}

void IpcMessage::SetErrorCode(uint32_t error_code) {
        error_code = error_code;
        if (raw_ptr) {
                raw_ptr[(payload_off >> 2) + 2] = error_code;
        }
}

namespace IPC {

static uint32_t handle_id;
static SmService sm;
std::unordered_map<std::string, IpcService *> services;
bool is_domainobj = false;
std::unordered_map<uint32_t, IpcService *> handles;

#define SERVICE(str, iface) do { services[str] = new iface(); } while(0)

void InitIPC() {
        sm.Initialize();
        SERVICE_MAPPING(); // From IpcStubs.hpp
}

uint32_t NewHandle(IpcService *srv) {
        handles[handle_id] = srv;
        return handle_id++;
}

uint32_t ConnectToPort(std::string name) {
        if (name != "sm:") {
                ns_abort("Attempt to connect to unknown service\n");
        }
        return NewHandle(&sm);
}

uint32_t ProcMessage(IpcService *handler, uint8_t buf[]) {
        uint8_t obuf[0x100];
        memset(obuf, 0, 0x100);
        IpcMessage req(buf, is_domainobj);
        req.ParseMessage();
        IpcMessage resp(obuf, is_domainobj);
        uint32_t ret = 0xf601;

        switch(req.type) {
        case 2: //Close
                resp.GenBuf(0, 0, 0);
                resp.SetErrorCode(0);
                ret = 0x25a0b;
                break;
        case 4: //Normal
                ret = handler->Dispatch(&req, &resp);
                break;
        case 5: //Control
                switch(req.cmd_id) {
                    case 0: // ConvertSessionToDomain
                        debug_print("IPC: ConvertSessionToDomain\n");
                        resp.GenBuf(0, 0, 4);
                        is_domainobj = true;
                        *resp.GetDataPointer<uint32_t*>(8) = handler->handle;
                        resp.SetErrorCode(0);
                        break;
                    case 2: // DuplicateSession
                        debug_print("DuplicateSession\n");
                        is_domainobj = false;
                        resp.GenBuf(1, 0, 0);
                        resp.SetMove(0, NewHandle(handler));
                        resp.SetErrorCode(0);
                        ret = 0;
                        break;
                    case 3: // QueryPointerBufferSize
                        debug_print("QueryPointerBufferSize\n");
                        resp.GenBuf(0, 0, 4);
                        *resp.GetDataPointer<uint32_t *>(8) = 0x500;
                        resp.SetErrorCode(0);
                        ret = 0;
                        break;
                    case 4: // DuplicateSession
                        debug_print("DuplicateSessionEx\n");
                        is_domainobj = false;
                        resp.GenBuf(1, 0, 0);
                        resp.SetMove(0, NewHandle(handler));
                        resp.SetErrorCode(0);
                        ret = 0;
                        break;
                    default:
                        ns_abort("Unknown cmdId to control %u\n", req.cmd_id);
                }
        }
        return 0;
}

};
