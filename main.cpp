#include <iostream>
#include <cstdlib>
#include <csignal>
#include "lispy_config.h"
#include "mpc.h"
#include "generated.hpp"
#include "lval.hpp"
#include "lenv.hpp"
#include "editline.hpp"

using std::cout;
using std::endl;
using std::string;

void exit_handler(int);

/* Create some parsers */
mpc_parser_t* Integer = mpc_new("integer");
mpc_parser_t* Decimal = mpc_new("decimal");
mpc_parser_t* Number = mpc_new("number");
mpc_parser_t* Symbol = mpc_new("symbol");
mpc_parser_t* Sexpr = mpc_new("sexpr");
mpc_parser_t* Qexpr = mpc_new("qexpr");
mpc_parser_t* Expr = mpc_new("expr");
mpc_parser_t* Lispy = mpc_new("lispy");

int main(int argc, char* argv[]) {
    struct sigaction sig_handler;
    sig_handler.sa_handler = exit_handler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, nullptr);

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT, language_grammar, Integer, Decimal, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    /* Print Version and Exit Information */
    cout << "Lispy Version " << LISPY_VERSION << endl;
    cout << "Press Ctrl+C to Exit\n" << endl;

    auto env = lenv();
    builtin::add_builtins(&env);

    while(true) {
        /* Output our prompt  and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval *result = lval::read((mpc_ast_t*)r.output);
            result = lval::eval(&env, result);
            cout << *result << endl;
            delete result;
            mpc_ast_delete((mpc_ast_t*)r.output);
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    return 0;
}

void exit_handler(int) {
    /* Undefine and Delete our Parsers */
    mpc_cleanup(8, Decimal, Integer, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    cout << "\nBye" << endl;
    exit(0);
}
