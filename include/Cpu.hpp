#ifndef _CPU_HPP
#define _CPU_HPP

namespace Cpu {

enum class State {
	Running = 0,
	PowerDown = 1,
};

void Init();

void Run();

void Stop();

void SetState(State _state);

State GetState();

void DumpMachine();

}
#endif
