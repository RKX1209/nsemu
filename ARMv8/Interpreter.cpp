/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"

Interpreter *Interpreter::inst = nullptr;

void Interpreter::Run() {
  debug_print ("Running with Interpreter\n");
  while (Cpu::GetState() == Cpu::State::Running) {
    
  }
}
