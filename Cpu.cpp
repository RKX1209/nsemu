/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
namespace Cpu {

static State state = State::PowerDown;
FILE *TraceOut;
bool DeepTrace;

void Init() {
	ARMv8::Init ();
        SVC::Init ();
        // TODO: Thread::Init()
}

void Run() {
	switch (state) {
	case State::Running:
		ARMv8::RunLoop ();
		break;
	case State::PowerDown:
		break;
	}
}

void SetState(State _state) {
	state = _state;
}

State GetState() {
	return state;
}

void DumpMachine() {
        if (is_debug()) {
                ARMv8::Dump ();
        }
        if (TraceOut)
                ARMv8::DumpJson (TraceOut, DeepTrace);
}

}
