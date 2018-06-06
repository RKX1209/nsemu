/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"
uint32_t SmService::Initialize() {
        return 0;
}

uint32_t SmService::GetService(ServiceName _name, IpcService *service) {
        std::string name = (char *) _name;
        if (IPC::services.find(name) == IPC::services.end()) {
                ns_print("Unknown service name %s\n", name.c_str());
                return 0xC15; //error code
        }
        service = IPC::services[name];
        return 0;
}

uint32_t SmService::RegisterService(ServiceName _name, IpcService *service) {
        std::string name = (char *) _name;
        IpcService *new_srv = new IpcService();
        debug_print("Registering service %s\n", _name);
        IPC::services[name] = new_srv;
        service = new_srv;
        return 0;
}

uint32_t SmService::UnregisterService(ServiceName _name) {
        std::string name = (char *) _name;
        auto it = IPC::services.find(name);
        if (it == IPC::services.end()) {
                ns_print("Unknown service name %s\n", _name);
                return 0xC15; //error code
        }
        IpcService *srv = it->second;
        debug_print("Unregistering service %s\n", _name);
        delete srv;
        IPC::services.erase(it);
        return 0;
}
