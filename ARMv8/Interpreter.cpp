/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

Interpreter *Interpreter::inst = nullptr;

int Interpreter::SingleStep() {
  uint32_t inst = ARMv8::ReadInst(PC);
  debug_print ("Run Code: 0x%08lx\n", inst);
  PC += sizeof(uint32_t);
  return 0;
}

void Interpreter::Run() {
  debug_print ("Running with Interpreter\n");
  while (Cpu::GetState() == Cpu::State::Running) {
    SingleStep();
  }
}
