#include "builtin.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <unordered_map>
#include "LEnv.hpp"
#include "LValue.hpp"
#include "Lispy.hpp"
#include "lval_error.hpp"

using std::cout;
using std::endl;
using std::string;

#define LVAL_OPERATOR_BASE(X, Y, E1, E2, E3, E4)                               \
    switch (X->type) {                                                         \
        case LValue::Type::decimal:                                            \
            switch (Y->type) {                                                 \
                case LValue::Type::decimal:                                    \
                    E1;                                                        \
                case LValue::Type::integer:                                    \
                    E2;                                                        \
                default:                                                       \
                    return Y;                                                  \
            }                                                                  \
        case LValue::Type::integer:                                            \
            switch (Y->type) {                                                 \
                case LValue::Type::decimal:                                    \
                    E3;                                                        \
                case LValue::Type::integer:                                    \
                    E4;                                                        \
                default:                                                       \
                    return Y;                                                  \
            }                                                                  \
        default:                                                               \
            return X;                                                          \
    }

#define LVAL_OPERATOR(OP, X, Y)                                                \
    LVAL_OPERATOR_BASE(X, Y, X->dec OP Y->dec; return X, X->dec OP Y->integ;   \
                       return X, X->type = LValue::Type::decimal;              \
                       X->dec = X->integ; X->dec OP Y->dec;                    \
                       return X, X->integ OP Y->integ; return X)

#define LVAL_BINARY_HANDLER(HANDLER, X, Y)                                     \
    LVAL_OPERATOR_BASE(X, Y, X->dec = HANDLER(X->dec, Y->dec);                 \
                       return X, X->dec = HANDLER(X->dec, Y->integ);           \
                       return X, X->type = LValue::Type::decimal;              \
                       X->dec = HANDLER(X->integ, Y->dec);                     \
                       return X, X->integ = HANDLER(X->integ, Y->integ);       \
                       return X)

#define LVAL_COMPARISON(OP, X, Y, A, B)                                        \
    comp = [X, Y]() {                                                          \
        auto a = X->A;                                                         \
        auto b = Y->B;                                                         \
        delete X;                                                              \
        delete Y;                                                              \
        return new LValue(a OP b);                                             \
    };                                                                         \
                                                                               \
    return comp();

#define LVAL_COMP_OPERATOR(OP, X, Y)                                           \
    function<LValue *()> comp;                                                 \
    LVAL_OPERATOR_BASE(X, Y, LVAL_COMPARISON(OP, X, Y, dec, dec),              \
                       LVAL_COMPARISON(OP, X, Y, dec, integ),                  \
                       LVAL_COMPARISON(OP, X, Y, integ, dec),                  \
                       LVAL_COMPARISON(OP, X, Y, integ, integ))

#define LASSERT(args, cond, err)                                               \
    if (!(cond)) {                                                             \
        auto msg = error(err);                                                 \
        delete args;                                                           \
        return msg;                                                            \
    }

#define LASSERT_NUM_ARGS(func, args, num)                                      \
    LASSERT(args, args->cells.size() == num,                                   \
            lerr::mismatched_num_args(func, args->cells.size(), num))

#define LASSERT_TYPE(func, args, cell, expected)                               \
    LASSERT(args, (cell)->type == expected,                                    \
            lerr::passed_incorrect_type(func, (cell)->type, expected))

#define LASSERT_TYPE2(func, args, cell, expected1, expected2)                  \
    LASSERT(args, (cell)->type == expected1 || (cell)->type == expected2,      \
            lerr::passed_incorrect_type(func, (cell)->type,                    \
                                        {expected1, expected2}))

#define LASSERT_NOT_EMPTY(func, args, cell)                                    \
    LASSERT(args, (cell)->cells.size() != 0, lerr::passed_nil_expr(func))

#define LASSERT_NOT_EMPTY_STRING(func, args, cell)                             \
    LASSERT(args, !(cell)->str.empty(), lerr::passed_empty_string(func))

#define LASSERT_NUMBER(func, args, cell)                                       \
    LASSERT(                                                                   \
        args,                                                                  \
        (cell)->type == LValue::Type::integer ||                               \
            (cell)->type == LValue::Type::decimal,                             \
        lerr::passed_incorrect_type(func, (cell)->type, LValue::Type::number))

namespace builtin {

using std::function;
using std::unordered_map;

auto error = LValue::error;

unordered_map<string, function<LValue *(LValue *, LValue *)>> operator_table = {
    {"+", add},      {"-", substract}, {"*", multiply},  {"/", divide},
    {"%", reminder}, {"^", power},     {"min", minimum}, {"max", maximum}};

void add_builtins(LEnv *e) {
    // Variable functions
    e->add_builtin_macro("def", def);
    e->add_builtin_macro("=", put);
    e->add_builtin_function("\\", func_lambda);
    e->add_builtin_function("\\!", macro_lambda);

    // Math functions
    e->add_builtin_function("+", ope("+"));
    e->add_builtin_function("-", ope("-"));
    e->add_builtin_function("*", ope("*"));
    e->add_builtin_function("/", ope("/"));
    e->add_builtin_function("%", ope("%"));
    e->add_builtin_function("^", ope("^"));
    e->add_builtin_function("min", ope("min"));
    e->add_builtin_function("max", ope("max"));

    // Comparison functions
    e->add_builtin_function("==", equals);
    e->add_builtin_function("!=", not_equals);
    e->add_builtin_function(">", ordering_op(">"));
    e->add_builtin_function("<", ordering_op("<"));
    e->add_builtin_function(">=", ordering_op(">="));
    e->add_builtin_function("<=", ordering_op("<="));
    e->add_builtin_function("if", if_);

    // List Functions
    e->add_builtin_function("head", head);
    e->add_builtin_function("tail", tail);
    e->add_builtin_function("list", list);
    e->add_builtin_function("eval", eval);
    e->add_builtin_function("join", join);
    e->add_builtin_function("cons", cons);
    e->add_builtin_function("len", len);
    e->add_builtin_function("init", init);

    // String functions
    e->add_builtin_function("load", load);
    e->add_builtin_function("print", print);
    e->add_builtin_function("error", make_error);
    e->add_builtin_function("read", read);
    e->add_builtin_function("show", show);

    // System functions
    e->add_builtin_function("exit", exit);

    // Atoms
    LValue *True = new LValue(true);
    LValue *False = new LValue(false);
    e->def("true", True);
    e->def("false", False);
    delete True;
    delete False;
}

void add_builtin_commands(LEnv *e) {
    e->add_builtin_command(".clear", repl::clear);
    e->add_builtin_command(".printenv", repl::print_env);
    e->add_builtin_command(".quit", repl::quit);
}

lbuiltin ope(const string &op) {
    using namespace std::placeholders;
    return std::bind(handle_op, _1, _2, op);
}

LValue *handle_op(LEnv *e, LValue *a, const string &op) {
    for (auto cell: a->cells) {
        LASSERT_NUMBER(op, a, cell)
    }

    auto x = a->pop_first();
    if (op == "-" && a->cells.empty()) {
        x = negate(x);
    }

    auto op_it = operator_table.find(op);
    auto handler = op_it->second;

    while (!a->cells.empty()) {
        auto y = a->pop_first();
        x = handler(x, y);
        delete y;

        if (x->type == LValue::Type::error) break;
    }

    delete a;
    return x;
}

LValue *add(LValue *x, LValue *y){LVAL_OPERATOR(+=, x, y)}

LValue *substract(LValue *x, LValue *y){LVAL_OPERATOR(-=, x, y)}

LValue *multiply(LValue *x, LValue *y){LVAL_OPERATOR(*=, x, y)}

LValue *err_div_zero(LValue *x) {
    delete x;
    return error(lerr::div_zero());
}

LValue *err_int_mod(LValue *x) {
    delete x;
    return error(lerr::int_mod());
}

LValue *divide(LValue *x, LValue *y) {
    auto e1 = [x, y]() {
        x->dec /= y->dec;
        return x;
    };
    auto e2 = [x, y]() {
        x->dec /= y->integ;
        return x;
    };
    auto e3 = [x, y]() {
        x->type = LValue::Type::decimal;
        x->dec = x->integ;
        x->dec /= y->dec;
        return x;
    };
    auto e4 = [x, y]() {
        x->integ /= y->integ;
        return x;
    };

    LVAL_OPERATOR_BASE(x, y, return y->dec == 0 ? err_div_zero(x) : e1(),
                       return y->integ == 0 ? err_div_zero(x) : e2(),
                       return y->dec == 0 ? err_div_zero(x) : e3(),
                       return y->integ == 0 ? err_div_zero(x) : e4())
}

LValue *reminder(LValue *x, LValue *y) {
    auto e4 = [x, y]() {
        x->integ %= y->integ;
        return x;
    };

    LVAL_OPERATOR_BASE(x, y, return err_int_mod(x), return err_int_mod(x),
                       return err_int_mod(x),
                       return y->integ == 0 ? err_div_zero(x) : e4())
}

LValue *power(LValue *x, LValue *y){LVAL_BINARY_HANDLER(pow, x, y)}

LValue *negate(LValue *x) {
    switch (x->type) {
        case LValue::Type::decimal:
            x->dec *= -1;
            return x;
        case LValue::Type::integer:
            x->integ *= -1;
            return x;
        default:
            return x;
    }
}

LValue *min_max(std::function<bool(double, double)> comp, LValue *x,
                LValue *y) {
    auto e1 = [x, y]() {
        x->dec = y->dec;
        return x;
    };
    auto e2 = [x, y]() {
        x->dec = y->integ;
        return x;
    };
    auto e3 = [x, y]() {
        x->type = LValue::Type::decimal;
        x->dec = y->dec;
        return x;
    };
    auto e4 = [x, y]() {
        x->integ = y->integ;
        return x;
    };

    LVAL_OPERATOR_BASE(x, y, return comp(x->dec, y->dec) ? x : e1(),
                       return comp(x->dec, y->integ) ? x : e2(),
                       return comp(x->integ, y->dec) ? x : e3(),
                       return comp(x->integ, y->integ) ? x : e4())
}

LValue *minimum(LValue *x, LValue *y) {
    return min_max(std::less_equal<double>(), x, y);
}

LValue *maximum(LValue *x, LValue *y) {
    return min_max(std::greater_equal<double>(), x, y);
}

lbuiltin ordering_op(const string &op) {
    using namespace std::placeholders;
    return std::bind(ord, _1, _2, op);
}

LValue *ord(LEnv *e, LValue *a, const std::string &op) {
    LASSERT_NUM_ARGS(op, a, 2)
    auto begin = a->cells.begin();

    LASSERT_NUMBER(op, a, *begin)
    ++begin;
    LASSERT_NUMBER(op, a, *begin)

    auto x = a->pop_first();
    auto y = a->pop_first();
    delete a;

    if (op == "==") {
        LVAL_COMP_OPERATOR(==, x, y)
    } else if (op == "!=") {
        LVAL_COMP_OPERATOR(!=, x, y)
    } else if (op == ">") {
        LVAL_COMP_OPERATOR(>, x, y)
    } else if (op == "<") {
        LVAL_COMP_OPERATOR(<, x, y)
    } else if (op == ">=") {
        LVAL_COMP_OPERATOR(>=, x, y)
    } else if (op == "<=") {
        LVAL_COMP_OPERATOR(<=, x, y)
    }

    return error("Fatal! Weird operator '" + op + "' in ord");
}

LValue *cmp(LEnv *e, LValue *a, const std::string &op) {
    LASSERT_NUM_ARGS(op, a, 2)
    auto begin = a->cells.begin();

    auto x = *begin++;
    auto y = *begin;

    bool result = false;

    if (op == "==")
        result = *x == *y;
    else if (op == "!=")
        result = *x != *y;
    delete a;

    return new LValue(result);
}

LValue *equals(LEnv *e, LValue *a) { return cmp(e, a, "=="); }

LValue *not_equals(LEnv *e, LValue *a) { return cmp(e, a, "!="); }

LValue *if_(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("if", a, 3)
    auto begin = a->cells.begin();

    LASSERT_TYPE("if", a, *begin, LValue::Type::boolean)
    ++begin;
    LASSERT_TYPE("if", a, *begin, LValue::Type::qexpr)
    ++begin;
    LASSERT_TYPE("if", a, *begin, LValue::Type::qexpr)

    auto cond = a->pop_first();
    bool boolean = cond->boolean;
    delete cond;
    auto result = LValue::take(a, boolean ? 0 : 1);
    return LValue::eval_qexpr(e, result);
}

LValue *qexpr_head(LValue *a, LValue::iter begin) {
    LASSERT_NOT_EMPTY("head", a, *begin)

    auto v = LValue::take(a, begin);
    while (v->cells.size() > 1) {
        delete v->pop(1);
    }

    return v;
}

LValue *string_head(LValue *a, LValue::iter begin) {
    LASSERT_NOT_EMPTY_STRING("head", a, *begin)

    auto v = LValue::take(a, begin);
    v->str = v->str.substr(0, 1);

    return v;
}

LValue *head(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("head", a, 1)

    auto begin = a->cells.begin();

    LASSERT_TYPE2("head", a, *begin, LValue::Type::qexpr, LValue::Type::string)

    if ((*begin)->type == LValue::Type::qexpr)
        return qexpr_head(a, begin);
    else
        return string_head(a, begin);
}

LValue *qexpr_tail(LValue *a, LValue::iter begin) {
    LASSERT_NOT_EMPTY("tail", a, *begin)

    auto v = LValue::take(a, begin);
    delete v->pop_first();
    return v;
}

LValue *string_tail(LValue *a, LValue::iter begin) {
    LASSERT_NOT_EMPTY_STRING("tail", a, *begin)

    auto v = LValue::take(a, begin);
    v->str = v->str.substr(1);

    return v;
}

LValue *tail(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("tail", a, 1)

    auto begin = a->cells.begin();

    LASSERT_TYPE2("tail", a, *begin, LValue::Type::qexpr, LValue::Type::string)

    if ((*begin)->type == LValue::Type::qexpr)
        return qexpr_tail(a, begin);
    else
        return string_tail(a, begin);
}

LValue *list(LEnv *e, LValue *a) {
    a->type = LValue::Type::qexpr;
    return a;
}

LValue *eval(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("eval", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("eval", a, *begin, LValue::Type::qexpr)

    auto x = LValue::take(a, begin);
    return LValue::eval_qexpr(e, x);
}

LValue *qexpr_join(LValue *a) {
    auto x = a->pop_first();
    for (auto expr: a->cells) {
        x->cells.splice(x->cells.end(), expr->cells);
    }

    return x;
}

LValue *string_join(LValue *a) {
    auto x = a->pop_first();
    for (auto expr: a->cells) {
        x->str += expr->str;
    }

    return x;
}

LValue *join(LEnv *e, LValue *a) {
    auto it = a->cells.begin();
    auto first = *it;
    LASSERT_TYPE2("join", a, first, LValue::Type::qexpr, LValue::Type::string)

    for (++it; it != a->cells.end(); ++it) {
        LASSERT_TYPE("join", a, *it, first->type)
    }

    LValue *x;
    if (first->type == LValue::Type::qexpr)
        x = qexpr_join(a);
    else
        x = string_join(a);

    delete a;
    return x;
}

LValue *cons(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("cons", a, 2)
    auto it = ++a->cells.begin();

    LASSERT_TYPE("cons", a, *it, LValue::Type::qexpr)

    auto x = a->pop_first();
    auto v = a->pop_first();
    v->cells.push_front(x);
    delete a;

    return v;
}

LValue *len(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("len", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE2("len", a, *begin, LValue::Type::qexpr, LValue::Type::string)

    auto x = LValue::take(a, begin);
    auto length =
        new LValue(x->type == LValue::Type::qexpr ? (long)x->cells.size()
                                                  : (long)x->str.size());
    delete x;
    return length;
}

LValue *init(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("init", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("init", a, *begin, LValue::Type::qexpr)
    LASSERT(a, (*begin)->cells.size() != 0, lerr::passed_nil_expr("init"))

    auto v = LValue::take(a, begin);
    auto end = v->cells.end();
    end--;
    delete v->pop(end);
    return v;
}

LValue *var(LEnv *e, LValue *a, const string &func) {
    auto begin = a->cells.begin();

    if ((*begin)->type == LValue::Type::sexpr) {
        *begin = LValue::eval_sexpr(e, *begin);
    }

    LASSERT_TYPE2(func, a, *begin, LValue::Type::symbol, LValue::Type::qexpr)

    LValue *syms;

    if ((*begin)->type == LValue::Type::qexpr) {
        syms = *begin;
        for (auto cell: syms->cells) {
            LASSERT(a, cell->type == LValue::Type::symbol,
                    lerr::cant_define_non_sym(func, cell->type))
        }
    } else {
        *begin = LValue::qexpr({*begin});
        syms = *begin;
    }

    LASSERT(a, syms->cells.size() == a->cells.size() - 1,
            lerr::cant_define_mismatched_values(func));

    syms = a->pop_first();
    a = LValue::eval_cells(e, a);

    if (a->type == LValue::Type::error) {
        delete syms;
        return a;
    }

    auto val_it = a->cells.begin();
    for (auto sym_it = syms->cells.begin(); sym_it != syms->cells.end();
         ++sym_it, ++val_it) {
        if (func == "def") {
            e->def((*sym_it)->sym, *val_it);
        } else if (func == "=") {
            e->put((*sym_it)->sym, *val_it);
        }
    }

    delete a;
    delete syms;
    return LValue::sexpr();
}

LValue *def(LEnv *e, LValue *a) { return var(e, a, "def"); }

LValue *put(LEnv *e, LValue *a) { return var(e, a, "="); }

LValue *lambda(LEnv *e, LValue *a, const string &func) {
    LASSERT_NUM_ARGS(func, a, 2)
    auto begin = a->cells.begin();

    LASSERT_TYPE(func, a, *begin, LValue::Type::qexpr)
    ++begin;
    LASSERT_TYPE(func, a, *begin, LValue::Type::qexpr)

    auto syms = a->cells.front();
    for (auto cell: syms->cells) {
        LASSERT(a, cell->type == LValue::Type::symbol,
                lerr::cant_define_non_sym(func, cell->type))
    }

    auto formals = a->pop_first();
    auto body = a->pop_first();
    delete a;

    if (func == "\\")
        return LValue::function(formals, body);
    else
        return LValue::macro(formals, body);
}

LValue *func_lambda(LEnv *e, LValue *a) { return lambda(e, a, "\\"); }

LValue *macro_lambda(LEnv *e, LValue *a) { return lambda(e, a, "\\!"); }

LValue *load(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("load", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("load", a, *begin, LValue::Type::string)

    mpc_result_t r;
    if (mpc_parse_contents((*begin)->str.c_str(), Lispy::instance()->parser(),
                           &r)) {
        LValue *expr = LValue::read((mpc_ast_t *)r.output);
        mpc_ast_delete((mpc_ast_t *)r.output);
        while (!expr->cells.empty()) {
            auto x = LValue::eval(e, expr->pop_first());
            if (x->type == LValue::Type::error) {
                delete expr;
                delete a;

                return x;
            }

            delete x;
        }

        delete expr;
        delete a;

        return LValue::sexpr();
    } else {
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        auto err = error(lerr::could_not_load_library(err_msg));
        free(err_msg);
        delete a;

        return err;
    }
}

LValue *print(LEnv *e, LValue *a) {
    for (auto cell: a->cells) {
        cout << *cell << ' ';
    }

    cout << endl;
    delete a;

    return LValue::sexpr();
}

LValue *make_error(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("error", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("error", a, *begin, LValue::Type::string)

    LValue *err = error((*begin)->str);

    delete a;
    return err;
}

LValue *read(LEnv *e, LValue *a) { return read_file(e, a, "<read>"); }

LValue *read_file(LEnv *e, LValue *a, const string &filename) {
    LASSERT_NUM_ARGS("read", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("read", a, *begin, LValue::Type::string)

    mpc_result_t r;
    if (mpc_parse(filename.c_str(), (*begin)->str.c_str(),
                  Lispy::instance()->parser(), &r)) {
        LValue *result = LValue::read((mpc_ast_t *)r.output);
        result->type = LValue::Type::qexpr;

        mpc_ast_delete((mpc_ast_t *)r.output);
        delete a;

        return result;
    } else {
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        auto err = error(lerr::could_not_load_library(err_msg));
        free(err_msg);
        delete a;

        return err;
    }
}

LValue *show(LEnv *e, LValue *a) {
    for (auto cell: a->cells) {
        LASSERT_TYPE("show", a, cell, LValue::Type::string)
    }

    for (auto it = a->cells.begin(); it != a->cells.end();) {
        cout << (*it)->str;
        if (++it == a->cells.end()) {
            cout << endl;
        } else {
            cout << ' ';
        }
    }

    delete a;

    return LValue::sexpr();
}

LValue *exit(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("exit", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE2("exit", a, *begin, LValue::Type::string,
                  LValue::Type::integer)

    auto val = LValue::take_first(a);
    LValue *err = new LValue(LValue::Type::error);
    if (val->type == LValue::Type::integer) {
        err->integ = val->integ;
        err->err = "";
    } else {
        err->integ = 1;
        err->err = val->str;
    }

    auto lispy = Lispy::instance();
    lispy->flags |= LISPY_FLAG_EXIT;

    delete val;

    return err;
}

namespace repl {

LValue *clear(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("clear", a, 0)

    auto lispy = Lispy::instance();
    lispy->flags |= LISPY_FLAG_CLEAR_OUTPUT;

    return LValue::sexpr();
}

LValue *print_env(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("printenv", a, 0)
    for (auto entry: e->symbols) {
        cout << entry.first << ": " << *entry.second << "\n";
    }

    cout << std::endl;

    delete a;
    return LValue::sexpr();
}

LValue *quit(LEnv *e, LValue *a) {
    LASSERT_NUM_ARGS("quit", a, 0)

    auto lispy = Lispy::instance();
    lispy->flags |= LISPY_FLAG_EXIT;

    return LValue::sexpr();
}

} // namespace repl

} // namespace builtin
