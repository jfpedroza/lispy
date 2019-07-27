#include <iostream>
#include <cstdlib>
#include <csignal>
#include "lispy_config.h"
#include "mpc.h"
#include "generated.hpp"
#include "lval.hpp"
#include "lenv.hpp"
#include <linenoise.h>

using std::cout;
using std::endl;
using std::string;

bool load_prelude();
void cleanup();
void exit_handler(int);
void completion_hook(char const *prefix, linenoiseCompletions *lc);

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

lenv *env = nullptr;

int main(int argc, char* argv[]) {
    linenoiseInstallWindowChangeHandler();

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

    auto e = lenv();
    env = &e;
    builtin::add_builtins(env);

    // Load prelude
    if (!load_prelude()) {
        return 1;
    }

    // Interactive prompt
    if (argc == 1) {
        linenoiseSetCompletionCallback(completion_hook);

        /* Print Version and Exit Information */
        cout << "Lispy Version " << LISPY_VERSION << endl;
        cout << "Press Ctrl+C to Exit\n" << endl;

        auto prompt = "\x1b[1;32mlispy\x1b[0m> ";

        while(true) {
            /* Output our prompt  and get input */
            char *input = linenoise(prompt);
            if (!input) break;
            else if (*input == '\0') {
                free(input);
                break;
            }

            /* Add input to history */
            linenoiseHistoryAdd(input);

            /* Attempt to Parse the user Input */
            mpc_result_t r;
            if (mpc_parse("<stdin>", input, Lispy, &r)) {
                lval *result = lval::read((mpc_ast_t*)r.output);
                result = lval::eval(env, result);
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

        cout << "\nBye" << endl;
        linenoiseHistoryFree();
    } else { // List of files to load
        for (int i = 1; i < argc; ++i) {
            lval *args = lval::sexpr({ new lval(string(argv[i])) });
            lval *x = builtin::load(env, args);

            if (x->type == lval_type::error) {
                cout << *x << endl;
            }

            delete x;
        }
    }

    cleanup();
    return 0;
}

bool load_prelude() {
    lval *args = lval::sexpr({ new lval(string(prelude)) });
    lval *expr = builtin::read_file(env, args, "prelude.lspy");

    if (expr->type == lval_type::error) {
        cout << *expr << endl;
        delete expr;
        cleanup();
        return false;
    }

    while (!expr->cells.empty()) {
        auto x = lval::eval(env, expr->pop_first());
        if (x->type == lval_type::error) {
            cout << "Failed to load prelude: " << *x << endl;
            delete x;
            delete expr;
            cleanup();
            return false;
        }

        delete x;
    }

    delete expr;
    return true;
}

void cleanup() {
    /* Undefine and Delete our Parsers */
    mpc_cleanup(10, Decimal, Integer, Number, Symbol, String, Sexpr, Qexpr, Expr, Comment, Lispy);
}

void exit_handler(int) {
    cleanup();
    linenoiseHistoryFree();
    cout << "\nBye" << endl;
    exit(0);
}

void completion_hook(char const *prefix, linenoiseCompletions *lc) {
    auto symbols = env->keys(prefix);
    for (auto sym: symbols) {
        linenoiseAddCompletion(lc, sym->c_str());
    }
}
