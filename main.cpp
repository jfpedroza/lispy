#include <iostream>
#include <cstdlib>
#include "lispy_config.h"
#include "mpc.h"
#include "lval.hpp"
#include "editline.hpp"

using std::cout;
using std::endl;

int main(int argc, char* argv[]) {

    /* Create some parsers */
    mpc_parser_t* Integer = mpc_new("integer");
    mpc_parser_t* Decimal = mpc_new("decimal");
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                   \
        integer  : /-?[0-9]+/ ;                            \
        decimal  : /-?[0-9]+\\.[0-9]+/ ;                   \
        number   : <decimal> | <integer>;                   \
        symbol   : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"rem\" | \"pow\" | \"min\" | \"max\" ; \
        sexpr    : '(' <expr>* ')' ; \
        expr     : <number> | <symbol> | <sexpr> ;  \
        lispy    : /^/ <expr>* /$/ ;             \
        ",
        Integer, Decimal, Number, Symbol, Sexpr, Expr, Lispy);

    /* Print Version and Exit Information */
    cout << "Lispy Version 0.0.0.0.5" << endl;
    cout << "Press Ctrl+c to Exit\n" << endl;

    while(true) {
        /* Output our prompt  and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval *result = lval::read((mpc_ast_t*)r.output);
            result = lval::eval(result);
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

    /* Undefine and Delete our Parsers */
    mpc_cleanup(7, Decimal, Integer, Number, Symbol, Sexpr, Expr, Lispy);
    return 0;
}

