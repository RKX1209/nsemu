/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
uint32_t SmService::Initialize() {
        return 0;
}

uint32_t SmService::Dispatch(IpcMessage *req, IpcMessage *resp) {
        switch(req->cmd_id) {
                case 0: {
                        resp->GenBuf(0, 0, 0);
        		debug_print("IPC message to SmService::Dispatch\n");
        		resp->error_code = Initialize();
        		return 0;
        	}
        	case 1: {
        		resp->GenBuf(1, 0, 0);
        		IpcService service;
                        std::string name = ARMv8::ReadString(req->GetDataPointer<uint64_t >(8));
        		debug_print("IPC message to SmService::GetService: ServiceName name = %s\n", name.c_str());
        	        resp->error_code = GetService(name, &service);
        		if(!resp->error_code)
                                resp->SetMove(0, IPC::NewHandle(&service));
        		return 0;
        	}
        	case 2: {
        		resp->GenBuf(1, 0, 0);
        		IpcService service;
                        std::string name = ARMv8::ReadString(req->GetDataPointer<uint64_t>(8));
        		debug_print("[TODO] IPC message to SmService::RegisterService: ServiceName name = %s\n", name.c_str());
                        /* TODO: Currently hombrew applications with libtransistor doesn't use RegisterService message. */
        	        resp->error_code = RegisterService(name, &service);
        		if(!resp->error_code)
        			resp->SetMove(0, IPC::NewHandle(&service));
        		return 0;
        	}
        	case 3: {
        		resp->GenBuf(0, 0, 0);
                        std::string name = ARMv8::ReadString(req->GetDataPointer<uint64_t>(8));
        		debug_print("[TODO] IPC message to SmService::UnregisterService: ServiceName name = %s\n", name.c_str());
                        /* TODO: */
        	        //resp.error_code = UnregisterService(req.GetDataPointer<ServiceName>(8));
        		return 0;
        	}
        	default:
        		ns_abort("Unknown message cmdId %u to interface SmService", req->cmd_id);
        }
}
uint32_t SmService::GetService(std::string name, IpcService *service) {
        if (IPC::services.find(name) == IPC::services.end()) {
                ns_print("Unknown service name %s\n", name.c_str());
                return 0xC15; //error code
        }
        *service = IPC::services[name];
        return 0;
}

uint32_t SmService::RegisterService(std::string name, IpcService *service) {
        return 0;
}
