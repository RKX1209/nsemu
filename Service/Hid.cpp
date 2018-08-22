/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"

static uint8_t hid_shared_mem[0x40000];
uint32_t nn::hid::IHidServer::CreateAppletResource(nn::applet::AppletResourceUserId pid, uint64_t uid, nn::hid::IAppletResource*& res) {
        res = new IAppletResource();
        return 0;
}

uint32_t nn::hid::IAppletResource::GetSharedMemoryHandle(uint8_t *&handle) {
	ns_print("nn::hid::IAppletResource::GetSharedMemoryHandle\n");
        handle = hid_shared_mem;
	return 0;
}
