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

void cleanup();
void exit_handler(int);

/* Create some parsers */
mpc_parser_t* Integer;
mpc_parser_t* Decimal;
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Comment;
mpc_parser_t* Lispy;

int main(int argc, char* argv[]) {
    struct sigaction sig_handler;
    sig_handler.sa_handler = exit_handler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, nullptr);

    Integer = mpc_new("integer");
    Decimal = mpc_new("decimal");
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Comment = mpc_new("comment");
    Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT, language_grammar, Integer, Decimal, Number, Symbol, String, Sexpr, Qexpr, Expr, Comment, Lispy);

    auto env = lenv();
    builtin::add_builtins(&env);

    // Load prelude
    /*{
        lval *args = lval::sexpr({ new lval(string(prelude)) });
        lval *x = builtin::read_file(&env, args, "prelude.lspy");

        if (x->type == lval_type::error) {
            cout << *x << endl;
            delete x;
            cleanup();
            return 1;
        }

        delete x;
    }*/

    // Interactive prompt
    if (argc == 1) {
        /* Print Version and Exit Information */
        cout << "Lispy Version " << LISPY_VERSION << endl;
        cout << "Press Ctrl+C to Exit\n" << endl;

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
    } else { // List of files to load
        for (int i = 1; i < argc; ++i) {
            lval *args = lval::sexpr({ new lval(string(argv[i])) });
            lval *x = builtin::load(&env, args);

            if (x->type == lval_type::error) {
                cout << *x << endl;
            }

            delete x;
        }
    }

    cleanup();
    return 0;
}

void cleanup() {
    /* Undefine and Delete our Parsers */
    mpc_cleanup(10, Decimal, Integer, Number, Symbol, String, Sexpr, Qexpr, Expr, Comment, Lispy);
}

void exit_handler(int) {
    cleanup();
    cout << "\nBye" << endl;
    exit(0);
}
