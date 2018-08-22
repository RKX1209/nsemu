/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "Dispdrv.hpp"
#include "IpcStubs.hpp"

uint32_t nn::visrv::sf::IManagerRootService::GetDisplayService(uint32_t user, nn::visrv::sf::IApplicationDisplayService*& dis) {
	ns_print("nn::visrv::sf::IManagerRootService::GetDisplayService\n");
        dis = new nn::visrv::sf::IApplicationDisplayService();
	return 0;
}

uint32_t nn::visrv::sf::ISystemRootService::GetDisplayService(uint32_t user, nn::visrv::sf::IApplicationDisplayService*& dis) {
	ns_print("nn::visrv::sf::ISystemRootService::GetDisplayService\n");
        dis = new nn::visrv::sf::IApplicationDisplayService();
	return 0;
}
uint32_t nn::visrv::sf::IApplicationDisplayService::GetIndirectDisplayTransactionService(nns::hosbinder::IHOSBinderDriver*& drv) {
	ns_print("nn::visrv::sf::IApplicationDisplayService::GetIndirectDisplayTransactionService\n");
        drv = new nns::hosbinder::IHOSBinderDriver();
	return 0;
}
uint32_t nn::visrv::sf::IApplicationDisplayService::GetManagerDisplayService(nn::visrv::sf::IManagerDisplayService*& dis) {
	ns_print("nn::visrv::sf::IApplicationDisplayService::GetManagerDisplayService\n");
        dis = new nn::visrv::sf::IManagerDisplayService();
	return 0;
}
uint32_t nn::visrv::sf::IApplicationDisplayService::GetRelayService(nns::hosbinder::IHOSBinderDriver*& ihos) {
	ns_print("nn::visrv::sf::IApplicationDisplayService::GetRelayService\n");
        ihos = new nns::hosbinder::IHOSBinderDriver();
	return 0;
}
uint32_t nn::visrv::sf::IApplicationDisplayService::GetSystemDisplayService(nn::visrv::sf::ISystemDisplayService*& dis) {
	ns_print("nn::visrv::sf::IApplicationDisplayService::GetSystemDisplayService\n");
        dis = new nn::visrv::sf::ISystemDisplayService();
	return 0;
}
uint32_t nn::visrv::sf::IApplicationDisplayService::OpenDisplay(nn::vi::DisplayName _name, uint64_t& dispid) {
	ns_print("nn::visrv::sf::IApplicationDisplayService::OpenDisplay\n");
        char *name = (char *) _name;
        dispid = NVFlinger::OpenDisplay(std::string(name));
	return 0;
}
uint32_t nn::visrv::sf::IApplicationDisplayService::GetDisplayVsyncEvent(uint64_t dispid, Kernel::Event*& event) {
	ns_print("nn::visrv::sf::IApplicationDisplayService::GetDisplayVsyncEvent\n");
        event = NVFlinger::GetVsyncEvent(dispid);
	return 0;
}
