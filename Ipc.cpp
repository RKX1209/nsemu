/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

uint32_t SmService::Dispatch(IpcInMessage *req, IpcOutMessage *resp) {

}
uint32_t SmService::GetService(std::string name, IpcService *service) {
        if (IPC::services.find(name) == IPC::services.end()) {
                ns_print("Unknown service name %s\n", name.c_str());
                return 0xC15; //error code
        }
        *service = IPC::services[name];
        return 0;
}

namespace IPC {

static uint32_t handle_id;
static SmService sm;
std::unordered_map<std::string, IpcService> services;
static std::unordered_map<uint32_t, IpcService *> handles;

void Initialize() {
}

uint32_t NewHandle(IpcService *srv) {
        handles[handle_id] = srv;
        return handle_id++;
}
IpcService *GetHandle(uint32_t handle) {
        if (handles.find(handle) == handles.end()) {
                return nullptr;
        }
        return handles[handle];
}
uint32_t ConnectToPort(std::string name) {
        if (name != "sm:") {
                ns_abort("Attempt to connect to unknown service\n");
        }
        return NewHandle(&sm);
}

uint32_t ProcMessage(IpcService *handler, uint8_t buf[]) {
        return 0;
}

};
