#include "Lispy.hpp"
#include <linenoise.h>
#include "generated.hpp"
#include "lispy_config.h"
#include "LValue.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

Lispy *Lispy::_instance = nullptr;

Lispy::Lispy()
    : flags(LISPY_NO_FLAGS),
      exit_code(0),
      cmd_line("The Lispy interpreter", ' ', LISPY_VERSION),
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
              sexpr_parser, qexpr_parser, expr_parser, comment_parser,
              command_parser, lispy_parser);

    builtin::add_builtins(&env);
    _instance = this;
}

Lispy::~Lispy() {
    mpc_cleanup(11, integer_parser, decimal_parser, number_parser,
                symbol_parser, string_parser, sexpr_parser, qexpr_parser,
                expr_parser, comment_parser, command_parser, lispy_parser);
}

Lispy *Lispy::instance() { return _instance; }

mpc_parser_t *Lispy::parser() { return lispy_parser; }

int Lispy::run(int argc, char *argv[]) {
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
            flags |= LISPY_FLAG_INTERACTIVE;
            builtin::add_builtin_commands(&env);
            run_interactive();
        }
    } catch (TCLAP::ArgException &e) {
        cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
        return 1;
    }

    return exit_code;
}

bool Lispy::load_prelude() {
    LValue *args = LValue::sexpr({new LValue(string(prelude))});
    LValue *expr = builtin::read_file(&env, args, "prelude.lspy");

    if (expr->type == lval_type::error) {
        cerr << *expr << endl;
        delete expr;
        return false;
    }

    while (!expr->cells.empty()) {
        auto x = LValue::eval(&env, expr->pop_first());
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
    auto lispy = Lispy::instance();
    auto symbols = lispy->env.keys(prefix);
    for (auto sym: symbols) {
        linenoiseAddCompletion(lc, sym->c_str());
    }
}

void Lispy::run_interactive() {
    linenoiseInstallWindowChangeHandler();

    linenoiseSetCompletionCallback(completion_hook);

    /* Print Version and Exit Information */
    cout << "Lispy Version " << LISPY_VERSION << endl;
    cout << "Press Ctrl+C to Exit\n" << endl;

    auto prompt = "\x1b[1;32mlispy\x1b[0m> ";

    while (true) {
        char *input = linenoise(prompt);
        if (!input) break;

        linenoiseHistoryAdd(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, lispy_parser, &r)) {
            LValue *result = LValue::read((mpc_ast_t *)r.output);
            result = LValue::eval(&env, result);
            bool cont = process_result(result);
            delete result;
            mpc_ast_delete((mpc_ast_t *)r.output);
            if (!cont) break;
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    if (exit_code == 0) cout << "\nBye" << endl;
    linenoiseHistoryFree();
}

bool Lispy::process_result(LValue *result) {
    if (flags & LISPY_FLAG_CLEAR_OUTPUT) {
        linenoiseClearScreen();
        flags &= ~LISPY_FLAG_CLEAR_OUTPUT;
        return true;
    }

    if (flags & LISPY_FLAG_EXIT) {
        if (result->type == lval_type::error) {
            exit_code = result->integ;
            if (result->err != "") {
                cout << "Exiting with message: " << result->err << endl;
            }
        }

        return false;
    }

    if (flags & LISPY_FLAG_INTERACTIVE || result->type == lval_type::error) {
        cout << *result << endl;
    }

    if (result->type == lval_type::error && flags & LISPY_FLAG_FAIL_ON_ERROR) {
        return false;
    }

    return true;
}

bool Lispy::load_files(const vector<string> &files) {
    flags |= LISPY_FLAG_FAIL_ON_ERROR;

    for (auto file: files) {
        LValue *args = LValue::sexpr({new LValue(file)});
        LValue *x = builtin::load(&env, args);

        bool cont = process_result(x);
        delete x;
        if (!cont) return false;
    }

    flags &= ~LISPY_FLAG_FAIL_ON_ERROR;
    return true;
}

bool Lispy::eval_strings(const vector<string> &strings) {
    flags |= LISPY_FLAG_FAIL_ON_ERROR;

    for (auto str: strings) {
        LValue *args = LValue::sexpr({new LValue(str)});
        LValue *expr = builtin::read(&env, args);

        if (expr->type == lval_type::error) {
            cout << *expr << endl;
            delete expr;
            exit_code = 1;
            return false;
        }

        auto x = LValue::eval_qexpr(&env, expr);

        bool cont = process_result(x);
        delete x;
        if (!cont) return false;
    }

    flags &= ~LISPY_FLAG_FAIL_ON_ERROR;
    return true;
}
