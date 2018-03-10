/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
namespace Cpu {

static State state = State::PowerDown;
FILE *TraceOut;

void Init() {
	ARMv8::Init ();
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
        //ARMv8::Dump ();
        if (TraceOut)
                ARMv8::DumpJson (TraceOut);
}

}
