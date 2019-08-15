#include "lispy.hpp"
#include <linenoise.h>
#include "generated.hpp"
#include "lispy_config.h"
#include "lval.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

lispy *lispy::_instance = nullptr;

lispy::lispy()
    : cmd_line("The Lispy interpreter", ' ', LISPY_VERSION),
      interactive_arg("i", "interactive",
                      "Run REPL, even when -e is present or files are given",
                      false),
      eval_args("e", "eval", "Eval program given as string", false, "program"),
      file_args("files", "Read programs from scripts", false, "file") {
    integer_parser = mpc_new("integer");
    decimal_parser = mpc_new("decimal");
    number_parser = mpc_new("number");
    symbol_parser = mpc_new("symbol");
    string_parser = mpc_new("string");
    sexpr_parser = mpc_new("sexpr");
    qexpr_parser = mpc_new("qexpr");
    expr_parser = mpc_new("expr");
    comment_parser = mpc_new("comment");
    command_parser = mpc_new("cname");
    lispy_parser = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT, language_grammar, integer_parser,
              decimal_parser, number_parser, symbol_parser, string_parser,
              sexpr_parser, qexpr_parser, expr_parser, comment_parser, command_parser,
              lispy_parser);

    builtin::add_builtins(&env);
    _instance = this;
}

lispy::~lispy() {
    mpc_cleanup(11, integer_parser, decimal_parser, number_parser,
                symbol_parser, string_parser, sexpr_parser, qexpr_parser,
                expr_parser, comment_parser, command_parser, lispy_parser);
}

lispy *lispy::instance() { return _instance; }

mpc_parser_t *lispy::parser() { return lispy_parser; }

int lispy::run(int argc, char *argv[]) {
    // Load prelude
    if (!load_prelude()) {
        return 1;
    }

    try {
        cmd_line.add(interactive_arg);
        cmd_line.add(eval_args);
        cmd_line.add(file_args);
        cmd_line.parse(argc, argv);

        auto interactive = interactive_arg.getValue();
        auto evals = eval_args.getValue();
        auto files = file_args.getValue();

        if (!eval_strings(evals) || !load_files(files)) {
            return 1;
        }

        if ((evals.empty() && files.empty()) || interactive) {
            run_interactive();
        }
    } catch (TCLAP::ArgException &e) {
        cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    }

    return 0;
}

bool lispy::load_prelude() {
    lval *args = lval::sexpr({new lval(string(prelude))});
    lval *expr = builtin::read_file(&env, args, "prelude.lspy");

    if (expr->type == lval_type::error) {
        cerr << *expr << endl;
        delete expr;
        return false;
    }

    while (!expr->cells.empty()) {
        auto x = lval::eval(&env, expr->pop_first());
        if (x->type == lval_type::error) {
            cerr << "Failed to load prelude: " << *x << endl;
            delete x;
            delete expr;
            return false;
        }

        delete x;
    }

    delete expr;
    return true;
}

void completion_hook(char const *prefix, linenoiseCompletions *lc) {
    auto lspy = lispy::instance();
    auto symbols = lspy->env.keys(prefix);
    for (auto sym: symbols) {
        linenoiseAddCompletion(lc, sym->c_str());
    }
}

void lispy::run_interactive() {
    linenoiseInstallWindowChangeHandler();

    linenoiseSetCompletionCallback(completion_hook);

    /* Print Version and Exit Information */
    cout << "Lispy Version " << LISPY_VERSION << endl;
    cout << "Press Ctrl+C to Exit\n" << endl;

    auto prompt = "\x1b[1;32mlispy\x1b[0m> ";

    while (true) {
        char *input = linenoise(prompt);
        if (!input)
            break;
        else if (*input == '\0') {
            free(input);
            break;
        }

        linenoiseHistoryAdd(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, lispy_parser, &r)) {
            lval *result = lval::read((mpc_ast_t *)r.output);
            result = lval::eval(&env, result);
            cout << *result << endl;
            delete result;
            mpc_ast_delete((mpc_ast_t *)r.output);
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    cout << "\nBye" << endl;
    linenoiseHistoryFree();
}

bool lispy::load_files(const vector<string> &files) {
    for (auto file: files) {
        lval *args = lval::sexpr({new lval(file)});
        lval *x = builtin::load(&env, args);

        if (x->type == lval_type::error) {
            cout << *x << endl;
            delete x;
            return false;
        }

        delete x;
    }

    return true;
}

bool lispy::eval_strings(const vector<string> &strings) {
    for (auto str: strings) {
        lval *args = lval::sexpr({new lval(str)});
        lval *expr = builtin::read(&env, args);

        if (expr->type == lval_type::error) {
            cout << *expr << endl;
            delete expr;
            return false;
        }

        auto x = lval::eval_qexpr(&env, expr);
        cout << *x << endl;

        if (x->type == lval_type::error) {
            delete x;
            return false;
        }

        delete x;
    }

    return true;
}
