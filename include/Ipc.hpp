#ifndef _IPC_HPP
#define _IPC_HPP

class IpcMessage {
public:
        IpcMessage() {}
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
	T getDataPointer(uint offset) {
                return (T) (raw_ptr + payload_off + 8 + offset);
	}
        void GenBuf(unsigned int _move_cnt, unsigned int _copy_cnt, unsigned int _data_bytes);
        void SetErrorCode(uint32_t error_code);
        void ParseMessage();
private:
        unsigned int copy_off, move_off, desc_off, raw_off, payload_off, realdata_off;
        uint8_t *raw_ptr;
        bool is_domainobj;
};

class IpcService {
public:
        IpcService() {}
        virtual uint32_t Dispatch(IpcMessage *req, IpcMessage *resp) { return 0; }
};

class SmService : public IpcService {
public:
        SmService() { }
        uint32_t GetService(std::string name, IpcService *service);
        uint32_t Dispatch(IpcMessage *req, IpcMessage *resp);
};

namespace IPC {

extern std::unordered_map<std::string, IpcService> services;

extern bool is_domainobj;

void Initialize();

uint32_t NewHandle(IpcService *srv);

IpcService *GetHandle(uint32_t handle);

uint32_t ConnectToPort(std::string name);

uint32_t ProcMessage(IpcService *handler, uint8_t buf[]);

};

#endif
