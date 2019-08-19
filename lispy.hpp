#ifndef LISPY_HPP
#define LISPY_HPP

#include <linenoise.h>
#include <tclap/CmdLine.h>
#include "lenv.hpp"
#include "mpc.h"

#define LISPY_NO_FLAGS 0x0
#define LISPY_FLAG_INTERACTIVE 0x1
#define LISPY_FLAG_CLEAR_OUTPUT 0x2

class lispy {
   public:
    lispy();
    ~lispy();

    static lispy *instance();

    int run(int argc, char *argv[]);
    mpc_parser_t *parser();

    uint flags;

   private:
    lenv env;
    static lispy *_instance;

    bool load_prelude();
    void run_interactive();
    bool process_interactive_result(lval *result);
    bool load_files(const std::vector<std::string> &files);
    bool eval_strings(const std::vector<std::string> &strings);

    friend void completion_hook(char const *prefix, linenoiseCompletions *lc);

    // Grammar parsers
    mpc_parser_t *integer_parser;
    mpc_parser_t *decimal_parser;
    mpc_parser_t *number_parser;
    mpc_parser_t *symbol_parser;
    mpc_parser_t *string_parser;
    mpc_parser_t *sexpr_parser;
    mpc_parser_t *qexpr_parser;
    mpc_parser_t *expr_parser;
    mpc_parser_t *comment_parser;
    mpc_parser_t *command_parser;
    mpc_parser_t *lispy_parser;

    // Command line arguments parsing
    TCLAP::CmdLine cmd_line;
    TCLAP::SwitchArg interactive_arg;
    TCLAP::MultiArg<std::string> eval_args;
    TCLAP::UnlabeledMultiArg<std::string> file_args;
};

#endif // LISPY_HPP
