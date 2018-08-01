/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"
#include "Am.hpp"

uint32_t nn::am::service::IApplicationProxyService::OpenApplicationProxy(uint64_t reserved, uint64_t pid, IpcService* handle, IApplicationProxy*& proxy) {
	ns_print("nn::am::service::IApplicationProxyService::OpenApplicationProxy\n");
        proxy = new IApplicationProxy();
	return 0;
}

/* IApplicationProxy ... following proxies are used by regular apps */
uint32_t nn::am::service::IApplicationProxy::GetApplicationFunctions(IApplicationFunctions*& func) {
	ns_print("nn::am::service::IApplicationProxy::GetApplicationFunctions\n");
        func = new IApplicationFunctions();
	return 0;
}

uint32_t nn::am::service::IApplicationProxy::GetAudioController(IAudioController*& ctr) {
        ns_print("nn::am::service::IApplicationProxy::GetAudioController\n");
        ctr = new IAudioController();
        return 0;
}

uint32_t nn::am::service::IApplicationProxy::GetCommonStateGetter(ICommonStateGetter*& getter) {
        ns_print("nn::am::service::IApplicationProxy::GetCommonStateGetter\n");
        getter = new ICommonStateGetter();
        return 0;
}

uint32_t nn::am::service::IApplicationProxy::GetDebugFunctions(IDebugFunctions*& dfunc) {
        ns_print("nn::am::service::IApplicationProxy::GetDebugFunctions\n");
        dfunc = new IDebugFunctions();
        return 0;
}
uint32_t nn::am::service::IApplicationProxy::GetDisplayController(IDisplayController*& ctr) {
        ns_print("nn::am::service::IApplicationProxy::GetDisplayController\n");
        ctr = new IDisplayController();
        return 0;
}
uint32_t nn::am::service::IApplicationProxy::GetLibraryAppletCreator(ILibraryAppletCreator*& creator) {
        ns_print("nn::am::service::IApplicationProxy::GetLibraryAppletCreator\n");
        creator = new ILibraryAppletCreator();
        return 0;
}
uint32_t nn::am::service::IApplicationProxy::GetProcessWindingController(IProcessWindingController*& ctr) {
        ns_print("nn::am::service::IApplicationProxy::GetProcessWindingController\n");
        ctr = new IProcessWindingController();
        return 0;
}
uint32_t nn::am::service::IApplicationProxy::GetSelfController(ISelfController*& ctr) {
        ns_print("nn::am::service::IApplicationProxy::GetSelfController\n");
        ctr = new ISelfController();
        return 0;
}
uint32_t nn::am::service::IApplicationProxy::GetWindowController(IWindowController*& ctr) {
        ns_print("nn::am::service::IApplicationProxy::GetWindowController\n");
        ctr = new IWindowController();
        return 0;
}

/* ICommonStateGetter */
uint32_t nn::am::service::ICommonStateGetter::ReceiveMessage(nn::am::AppletMessage& msg) {
	ns_print("nn::am::service::ICommonStateGetter::ReceiveMessage\n");
	msg = 15;
	return 0;
}

uint32_t nn::am::service::ICommonStateGetter::GetCurrentFocusState(uint8_t& state) {
	ns_print("nn::am::service::ICommonStateGetter::GetCurrentFocusState\n");
	state = FocusState::InFocus;
	return 0;
}

/* ISelfController */
uint32_t nn::am::service::ISelfController::SetFocusHandlingMode(bool _0, bool _1, bool _2) {
	ns_print("nn::am::service::ISelfController::SetFocusHandlingMode\n");
	/* TODO */
	return 0;
}
uint32_t nn::am::service::ISelfController::SetOutOfFocusSuspendingEnabled(bool _0) {
	ns_print("nn::am::service::ISelfController::SetOutOfFocusSuspendingEnabled\n");
	/* TODO */
	return 0;
}
/* ISelfController */
