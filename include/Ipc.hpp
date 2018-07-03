#ifndef _IPC_HPP
#define _IPC_HPP
class KObject {
public:
        virtual ~KObject() {}
        virtual void Close() {}
};

class IpcMessage {
public:
        IpcMessage()  : raw_ptr(nullptr) {}
        IpcMessage(uint8_t *buf, bool is_domainobj) : raw_ptr(buf), is_domainobj(is_domainobj) {}
        /* IPC command structure */
        unsigned int type;
        unsigned int x_cnt, a_cnt, b_cnt, w_cnt, has_c;
        /* Handle descriptr (if enabled)*/
        unsigned int has_pid, pid, move_cnt, copy_cnt;
        /* Domain */
        unsigned int domain_handle, domain_cmd;
        /* Data */
        unsigned int cmd_id;

        uint32_t error_code; // Use only in response message

        template<typename T>
        T GetData(unsigned int offset) {
            return *((T *) (raw_ptr + payload_off + 8 + offset));
        }
        template<typename T>
        T GetDataPointer(uint offset) {
                ns_print("req.type=%u, raw_ptr:0x%x, payload_off:0x%x\n", type, raw_ptr, payload_off);
                return (T) (raw_ptr + payload_off + 8 + offset);
        }
        uint64_t GetBuffer(int btype, int num, unsigned int& size) {
                if(btype & 0x20) {
                        auto buf = GetBuffer((btype & ~0x20) | 4, num, size);
                        if(size != 0)
                                return buf;
                        return GetBuffer((btype & ~0x20) | 8, num, size);
                }
                size = 0;
                auto ax = (btype & 3) == 1;
                auto flags_ = btype & 0xC0;
                auto flags = flags_ == 0x80 ? 3 : (flags_ == 0x40 ? 1 : 0);
                auto cx = (btype & 0xC) == 8;
                switch((ax << 1) | cx) {
                        case 0: { // B
                                auto t = (uint32_t *) (raw_ptr + desc_off + x_cnt * 8 + a_cnt * 12 + num * 12);
                                uint64_t a = t[0], b = t[1], c = t[2];
                                size = (unsigned int) (a | (((c >> 24) & 0xF) << 32));
                                if((c & 0x3) != flags)
                                        ns_print("B descriptor flags don't match: %u vs expected %u", (unsigned int) (c & 0x3), flags);
                                return b | (((((c >> 2) << 4) & 0x70) | ((c >> 28) & 0xF)) << 32);
                        }
                        case 1: { // C
                                auto t = (uint32_t *) (raw_ptr + raw_off + w_cnt * 4);
                                uint64_t a = t[0], b = t[1];
                                size = b >> 16;
                                return a | ((b & 0xFFFF) << 32);
                        }
                        case 2: { // A
                                auto t = (uint32_t *) (raw_ptr + desc_off + x_cnt * 8 + num * 12);
                                uint64_t a = t[0], b = t[1], c = t[2];
                                size = (unsigned int) (a | (((c >> 24) & 0xF) << 32));
                                if((c & 0x3) != flags)
                                        ns_print("A descriptor flags don't match: %u vs expected %u", (uint) (c & 0x3), flags);
                                return b | (((((c >> 2) << 4) & 0x70) | ((c >> 28) & 0xF)) << 32);
                        }
                        case 3: { // X
                                auto t = (uint32_t *) (raw_ptr + desc_off + num * 8);
                                uint64_t a = t[0], b = t[1];
                                size = (unsigned int) (a >> 16);
                                return b | ((((a >> 12) & 0xF) | ((a >> 2) & 0x70)) << 32);
                        }
                }
                return 0;
        }
        void GenBuf(unsigned int _move_cnt, unsigned int _copy_cnt, unsigned int _data_bytes);
        void SetErrorCode(uint32_t error_code);
        void ParseMessage();
        void SetMove(int offset, uint32_t handler) {
                uint32_t *buf = (uint32_t *) raw_ptr;
                if (!raw_ptr) {
                        return;
                }
                ns_print("SetMove (0x%x)\n", handler);
                if(is_domainobj)
                        buf[(payload_off >> 2) + 4 + offset] = handler;
                else
            	        buf[3 + copy_cnt + offset] = handler;
        }
        void SetCopy(int offset, uint32_t handler) {
            if (!raw_ptr) {
                return;
            }
            uint32_t *buf = (uint32_t *) raw_ptr;
            buf[3 + offset] = handler;
        }
        uint32_t GetMoved(int off) {
		return *(uint32_t *) (raw_ptr + move_off + off * 4);
	}
	uint32_t GetCopied(int off) {
		return *(uint32_t *) (raw_ptr + copy_off + off * 4);
	}
private:
        unsigned int copy_off, move_off, desc_off, raw_off, payload_off, realdata_off;
        uint8_t *raw_ptr;
        bool is_domainobj;
};

class IpcService {
public:
        IpcService() : handle(0xf000){}
        virtual uint32_t Dispatch(IpcMessage *req, IpcMessage *resp) { return 0; }
        int handle;
};

class IUnknown : public IpcService {
};

namespace IPC {

extern std::unordered_map<std::string, IpcService*> services;
extern std::unordered_map<uint32_t, IpcService *> handles;
extern bool is_domainobj;

void InitIPC();

uint32_t NewHandle(IpcService *srv);

template<typename T>
T GetHandle(uint32_t handle) {
        if (handles.find(handle) == handles.end()) {
                return nullptr;
        }
        IpcService *srv = handles[handle];
        return static_cast<T>(srv);
}

uint32_t ConnectToPort(std::string name);

uint32_t ProcMessage(IpcService *handler, uint8_t buf[]);

};

#endif
