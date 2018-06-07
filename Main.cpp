/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include <csignal>
#include "optionparser.h"
using namespace std;
struct Arg : public option::Arg {
	static void printError(const char *msg1, const option::Option& opt, const char *msg2)
	{
		fprintf (stderr, "%s", msg1);
		fwrite (opt.name, opt.namelen, 1, stderr);
		fprintf (stderr, "%s", msg2);
	}
	static option::ArgStatus Unknown(const option::Option& option, bool msg)
	{
		if (msg) {
			printError ("Unknown option '", option, "'\n");
		}
		return option::ARG_ILLEGAL;
	}
	static option::ArgStatus Required(const option::Option& option, bool msg)
	{
		if (option.arg != nullptr) {
			return option::ARG_OK;
		}
		if (msg) {
			printError ("Option '", option, "' requires an argument\n");
		}
		return option::ARG_ILLEGAL;
	}
	static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
	{
		if (option.arg != nullptr && option.arg[0] != 0) {
			return option::ARG_OK;
		}
		if (msg) {
			printError ("Option '", option, "' requires a non-empty argument\n");
		}
		return option::ARG_ILLEGAL;
	}
	static option::ArgStatus Numeric(const option::Option& option, bool msg)
	{
		char *endptr = nullptr;
		if (option.arg != nullptr && strtol (option.arg, &endptr, 10)) {}
		;
		if (endptr != option.arg && *endptr == 0) {
			return option::ARG_OK;
		}
		if (msg) {
			printError ("Option '", option, "' requires a numeric argument\n");
		}
		return option::ARG_ILLEGAL;
	}
};

void InitTrace(const char *fname) {
        if ((Cpu::TraceOut = fopen(fname, "w")) == NULL) {
                ns_abort ("Can not open output file for trace\n");
        }
}

void FinTrace() {
        if (Cpu::TraceOut)
                fclose (Cpu::TraceOut);
}

void Banner() {

        const char *banner = "\n\n"\
        "███╗   ██╗███████╗███████╗███╗   ███╗██╗   ██╗\n"\
        "████╗  ██║██╔════╝██╔════╝████╗ ████║██║   ██║\n"\
        "██╔██╗ ██║███████╗█████╗  ██╔████╔██║██║   ██║\n"\
        "██║╚██╗██║╚════██║██╔══╝  ██║╚██╔╝██║██║   ██║\n"\
        "██║ ╚████║███████║███████╗██║ ╚═╝ ██║╚██████╔╝\n"\
        "╚═╝  ╚═══╝╚══════╝╚══════╝╚═╝     ╚═╝ ╚═════╝ \n\n"\
        "NSEMU ver (0.1: unstable)\n\n";
        ns_print(banner);
}

enum  optionIndex {
	UNKNOWN, HELP, ENABLE_TRACE, ENABLE_DEEP, ENABLE_GDB, ENABLE_DEBUG,
};
const option::Descriptor usage[] =
{
	{ UNKNOWN, 0, "", "", Arg::None, "USAGE: nsemu [options] <nso-binary>\n\n"
	  "Options:" },
	{ HELP, 0, "h", "help", Arg::None, "  --help  \tPrint help message" },
    { ENABLE_TRACE, 0, "t","enable-trace", Arg::None, "  --enable-trace, -t  \tEnable Trace" },
    { ENABLE_DEEP, 0, "","deep-trace", Arg::None, "  --deep-trace, -t  \tEnable Deep Trace" },
    { ENABLE_GDB, 0, "s","enable-gdb", Arg::None, "  --enable-gdb -s  \tEnable GDBServer" },
    { ENABLE_DEBUG, 0, "d","enable-debug", Arg::None, "  --enable-debug -d  \tEnable debug mode" },
	{ 0, 0, nullptr, nullptr, nullptr, nullptr }
};

static void SignalHandler(int sig, siginfo_t* sig_info, void* sig_data) {
        if(sig == SIGSEGV) {
                ns_print ("SEGV: %p\n", sig_info->si_addr );
                ARMv8::Dump();
                _Exit(-1);
        }
}

int main(int argc, char **argv) {
	Nsemu::create ();
	Nsemu *nsemu = Nsemu::get_instance ();
	argc -= argc > 0;
	argv += argc > 0;

	option::Stats stats (usage, argc, argv);
	vector<option::Option> options (stats.options_max);
	vector<option::Option> buffer (stats.buffer_max);
	option::Parser parse (usage, argc, argv, &options[0], &buffer[0]);

	if (parse.error ()) {
		return 1;
	} else {
		if (options[HELP].count () || options[UNKNOWN].count ()) {
printUsage:
			option::printUsage (cout, usage);
			return 0;
		}
        bool deep = options[ENABLE_DEEP].count () > 0;
        if (options[ENABLE_TRACE].count () > 0 || deep) {
			InitTrace ("nsemu_trace.json");
                        Cpu::DeepTrace = deep;
	}
        if (options[ENABLE_GDB].count () > 0) {
			GdbStub::Init();
	}
        if (options[ENABLE_DEBUG].count () > 0) {
			enable_debug();
	}

#if 0
		if (options[NSO].count () > 0) {
			if (options[NSO].count () != 1) {
				goto printUsage;
			}
			if (options[NRO].count () != 0) {
				goto printUsage;
			}
			if (parse.nonOptionsCount () != 0) {
				goto printUsage;
			}
		} else if (options[NRO].count () > 0) {
			if (options[NRO].count () != 1) {
				goto printUsage;
			}
			if (options[NSO].count () != 0) {
				goto printUsage;
			}
			if (parse.nonOptionsCount () != 0) {
				goto printUsage;
			}
		} else {
		}
#endif
		if (parse.nonOptionsCount () != 1) {
		              goto printUsage;
		}
	}
        /* ### Register SEGV handler for debugging ### */
        struct sigaction segv_act;
        sigemptyset(&segv_act.sa_mask);
        sigaddset(&segv_act.sa_mask, SIGSEGV);
        segv_act.sa_sigaction = SignalHandler;
        segv_act.sa_flags     = SA_SIGINFO|SA_RESTART|SA_ONSTACK;
        if( sigaction( SIGSEGV, &segv_act, NULL ) == -1 ){
                ns_abort ("Failed to set my signal handler.\n");
        }

        Banner ();
	nsemu->BootUp (parse.nonOption (0));
	Nsemu::destroy ();
        FinTrace ();
	return 0;
}
