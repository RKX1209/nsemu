#ifndef _INTERPRETER_HPP
#define _INTERPRETER_HPP

/* Global Interpreter singleton class .*/
class Interpreter {
private:
  Interpreter() = default;
  ~Interpreter() = default;

  static Interpreter *inst;
public:
  Interpreter(const Interpreter&) = delete;
  Interpreter& operator=(const Interpreter&) = delete;
  Interpreter(Interpreter&&) = delete;
  Interpreter& operator=(Interpreter&&) = delete;

  static Interpreter* get_instance() {
    return inst;
  }

  static void create() {
    if ( !inst ) {
      inst = new Interpreter;
    }
  }

  static void destroy() {
    if ( inst ) {
      delete inst;
      inst= nullptr;
    }
  }
  void Run();
};

#endif
