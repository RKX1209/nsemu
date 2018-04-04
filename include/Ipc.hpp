#ifndef _IPC_HPP
#define _IPC_HPP

class IpcInMessage {
public:
        IpcInMessage() {}
};

class IpcOutMessage {
public:
        IpcOutMessage() {}

};

class IpcService {
public:
        IpcService() {}
        virtual uint32_t Dispatch(IpcInMessage *req, IpcOutMessage *resp) {}
};

class SmService : public IpcService {
public:
        SmService() { }
        uint32_t GetService(std::string name, IpcService *service);
        uint32_t Dispatch(IpcInMessage *req, IpcOutMessage *resp);
};

namespace IPC {

extern std::unordered_map<std::string, IpcService> services;

void Initialize();

uint32_t NewHandle(IpcService *srv);

IpcService *GetHandle(uint32_t handle);

uint32_t ConnectToPort(std::string name);

uint32_t ProcMessage(IpcService *handler, uint8_t buf[]);

};

#endif
