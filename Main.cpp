/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "optionparser.h"
using namespace std;
struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s", msg2);
  }
  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != nullptr)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != nullptr && option.arg[0] != 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = nullptr;
    if (option.arg != nullptr && strtol(option.arg, &endptr, 10)){};
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }
};

enum  optionIndex { UNKNOWN, HELP };
const option::Descriptor usage[] =
{
	{UNKNOWN, 0, "", "",Arg::None, "USAGE: nsemu [options] <nso-binary>\n\n"
	                                       "Options:" },
	{HELP, 0,"","help",Arg::None, "  --help  \tUnsurprisingly, print this message." },
	{0,0,nullptr,nullptr,nullptr,nullptr}
};

int main(int argc, char **argv) {
  Nsemu::create();
  Nsemu *nsemu = Nsemu::get_instance();
  argc -= argc > 0;
  argv += argc > 0;

  option::Stats stats(usage, argc, argv);
  vector<option::Option> options(stats.options_max);
  vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

  if(parse.error()) {
		return 1;
	} else {
		if(options[HELP].count() || options[UNKNOWN].count()) {
		  printUsage:
			option::printUsage(cout, usage);
			return 0;
		}
    #if 0
		if(options[NSO].count() > 0) {
			if(options[NSO].count() != 1) { goto printUsage; }
			if(options[NRO].count() != 0) { goto printUsage; }
			if(parse.nonOptionsCount() != 0) { goto printUsage; }
		} else if(options[NRO].count() > 0) {
			if(options[NRO].count() != 1) { goto printUsage; }
			if(options[NSO].count() != 0) { goto printUsage; }
			if(parse.nonOptionsCount() != 0) { goto printUsage; }
		} else {
			if(parse.nonOptionsCount() != 1) { goto printUsage; }
		}
    #endif
	}
  nsemu->BootUp(parse.nonOption(0));
  Nsemu::destroy();
  return 0;
}