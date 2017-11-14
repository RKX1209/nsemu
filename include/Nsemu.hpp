#ifndef _NSEMU_HPP
#define _NSEMU_HPP

#include <stdint.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "Util.hpp"
#include "NintendoObject.hpp"

/* Global NSEMU singleton class .*/
class Nsemu {
private:
  Nsemu() = default;
  ~Nsemu() = default;

  static Nsemu *inst;
public:
  Nsemu(const Nsemu&) = delete;
  Nsemu& operator=(const Nsemu&) = delete;
  Nsemu(Nsemu&&) = delete;
  Nsemu& operator=(Nsemu&&) = delete;

  static Nsemu* get_instance() {
    return inst;
  }

  static void create() {
    if ( !inst ) {
      inst = new Nsemu;
    }
  }

  static void destroy() {
    if ( inst ) {
      delete inst;
      inst= nullptr;
    }
  }
};
#endif
