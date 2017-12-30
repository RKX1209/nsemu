#ifndef _INTERPRETER_HPP
#define _INTERPRETER_HPP

class IntprCallback : public DisasCallback {
public:
  void MoviI64(unsigned int reg_idx, uint64_t imm);
  /* Add/Sub with Immediate value */
  void AddiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64);
  void SubiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t imm, bool setflags, bool bit64);
  /* AND/OR/EOR... with Immediate value */
  void AndiI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool setflags, bool bit64);
  void OrrI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64);
  void EorI64(unsigned int rd_idx, unsigned int rn_idx, uint64_t wmask, bool bit64);
};

/* Global Interpreter singleton class .*/
class Interpreter {
private:
  Interpreter() = default;
  ~Interpreter() = default;

  static Interpreter *inst;
  static IntprCallback *disas_cb;
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
      inst->disas_cb = new IntprCallback;
    }
  }

  static void destroy() {
    if ( inst ) {
      delete inst->disas_cb;
      delete inst;
      inst= nullptr;
    }
  }
  void Run();
  int SingleStep();
};
#endif
