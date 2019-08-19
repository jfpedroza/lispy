#include "builtin.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <unordered_map>
#include "lenv.hpp"
#include "lispy.hpp"
#include "lval.hpp"
#include "lval_error.hpp"

using std::cout;
using std::endl;
using std::string;

#define LVAL_OPERATOR_BASE(X, Y, E1, E2, E3, E4)                               \
    switch (X->type) {                                                         \
        case lval_type::decimal:                                               \
            switch (Y->type) {                                                 \
                case lval_type::decimal:                                       \
                    E1;                                                        \
                case lval_type::integer:                                       \
                    E2;                                                        \
                default:                                                       \
                    return Y;                                                  \
            }                                                                  \
        case lval_type::integer:                                               \
            switch (Y->type) {                                                 \
                case lval_type::decimal:                                       \
                    E3;                                                        \
                case lval_type::integer:                                       \
                    E4;                                                        \
                default:                                                       \
                    return Y;                                                  \
            }                                                                  \
        default:                                                               \
            return X;                                                          \
    }

#define LVAL_OPERATOR(OP, X, Y)                                                \
    LVAL_OPERATOR_BASE(X, Y, X->dec OP Y->dec; return X, X->dec OP Y->integ;   \
                       return X, X->type = lval_type::decimal;                 \
                       X->dec = X->integ; X->dec OP Y->dec;                    \
                       return X, X->integ OP Y->integ; return X)

#define LVAL_BINARY_HANDLER(HANDLER, X, Y)                                     \
    LVAL_OPERATOR_BASE(X, Y, X->dec = HANDLER(X->dec, Y->dec);                 \
                       return X, X->dec = HANDLER(X->dec, Y->integ);           \
                       return X, X->type = lval_type::decimal;                 \
                       X->dec = HANDLER(X->integ, Y->dec);                     \
                       return X, X->integ = HANDLER(X->integ, Y->integ);       \
                       return X)

#define LVAL_COMPARISON(OP, X, Y, A, B)                                        \
    comp = [X, Y]() {                                                          \
        auto a = X->A;                                                         \
        auto b = Y->B;                                                         \
        delete X;                                                              \
        delete Y;                                                              \
        return new lval(a OP b);                                               \
    };                                                                         \
                                                                               \
    return comp();

#define LVAL_COMP_OPERATOR(OP, X, Y)                                           \
    function<lval *()> comp;                                                   \
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
        (cell)->type == lval_type::integer ||                                  \
            (cell)->type == lval_type::decimal,                                \
        lerr::passed_incorrect_type(func, (cell)->type, lval_type::number))

namespace builtin {

using std::function;
using std::unordered_map;

auto error = lval::error;

unordered_map<string, function<lval *(lval *, lval *)>> operator_table = {
    {"+", add},      {"-", substract}, {"*", multiply},  {"/", divide},
    {"%", reminder}, {"^", power},     {"min", minimum}, {"max", maximum}};

void add_builtins(lenv *e) {
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

    // Atoms
    lval *True = new lval(true);
    lval *False = new lval(false);
    e->def("true", True);
    e->def("false", False);
    delete True;
    delete False;
}

void add_builtin_commands(lenv *e) {
    e->add_builtin_command(".clear", repl::clear);
}

lbuiltin ope(const string &op) {
    using namespace std::placeholders;
    return std::bind(handle_op, _1, _2, op);
}

lval *handle_op(lenv *e, lval *a, const string &op) {
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

        if (x->type == lval_type::error) break;
    }

    delete a;
    return x;
}

lval *add(lval *x, lval *y){LVAL_OPERATOR(+=, x, y)}

lval *substract(lval *x, lval *y){LVAL_OPERATOR(-=, x, y)}

lval *multiply(lval *x, lval *y){LVAL_OPERATOR(*=, x, y)}

lval *err_div_zero(lval *x) {
    delete x;
    return error(lerr::div_zero());
}

lval *err_int_mod(lval *x) {
    delete x;
    return error(lerr::int_mod());
}

lval *divide(lval *x, lval *y) {
    auto e1 = [x, y]() {
        x->dec /= y->dec;
        return x;
    };
    auto e2 = [x, y]() {
        x->dec /= y->integ;
        return x;
    };
    auto e3 = [x, y]() {
        x->type = lval_type::decimal;
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

lval *reminder(lval *x, lval *y) {
    auto e4 = [x, y]() {
        x->integ %= y->integ;
        return x;
    };

    LVAL_OPERATOR_BASE(x, y, return err_int_mod(x), return err_int_mod(x),
                       return err_int_mod(x),
                       return y->integ == 0 ? err_div_zero(x) : e4())
}

lval *power(lval *x, lval *y){LVAL_BINARY_HANDLER(pow, x, y)}

lval *negate(lval *x) {
    switch (x->type) {
        case lval_type::decimal:
            x->dec *= -1;
            return x;
        case lval_type::integer:
            x->integ *= -1;
            return x;
        default:
            return x;
    }
}

lval *min_max(std::function<bool(double, double)> comp, lval *x, lval *y) {
    auto e1 = [x, y]() {
        x->dec = y->dec;
        return x;
    };
    auto e2 = [x, y]() {
        x->dec = y->integ;
        return x;
    };
    auto e3 = [x, y]() {
        x->type = lval_type::decimal;
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

lval *minimum(lval *x, lval *y) {
    return min_max(std::less_equal<double>(), x, y);
}

lval *maximum(lval *x, lval *y) {
    return min_max(std::greater_equal<double>(), x, y);
}

lbuiltin ordering_op(const string &op) {
    using namespace std::placeholders;
    return std::bind(ord, _1, _2, op);
}

lval *ord(lenv *e, lval *a, const std::string &op) {
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

lval *cmp(lenv *e, lval *a, const std::string &op) {
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

    return new lval(result);
}

lval *equals(lenv *e, lval *a) { return cmp(e, a, "=="); }

lval *not_equals(lenv *e, lval *a) { return cmp(e, a, "!="); }

lval *if_(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("if", a, 3)
    auto begin = a->cells.begin();

    LASSERT_TYPE("if", a, *begin, lval_type::boolean)
    ++begin;
    LASSERT_TYPE("if", a, *begin, lval_type::qexpr)
    ++begin;
    LASSERT_TYPE("if", a, *begin, lval_type::qexpr)

    auto cond = a->pop_first();
    bool boolean = cond->boolean;
    delete cond;
    auto result = lval::take(a, boolean ? 0 : 1);
    return lval::eval_qexpr(e, result);
}

lval *qexpr_head(lval *a, lval::iter begin) {
    LASSERT_NOT_EMPTY("head", a, *begin)

    auto v = lval::take(a, begin);
    while (v->cells.size() > 1) {
        delete v->pop(1);
    }

    return v;
}

lval *string_head(lval *a, lval::iter begin) {
    LASSERT_NOT_EMPTY_STRING("head", a, *begin)

    auto v = lval::take(a, begin);
    v->str = v->str.substr(0, 1);

    return v;
}

lval *head(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("head", a, 1)

    auto begin = a->cells.begin();

    LASSERT_TYPE2("head", a, *begin, lval_type::qexpr, lval_type::string)

    if ((*begin)->type == lval_type::qexpr)
        return qexpr_head(a, begin);
    else
        return string_head(a, begin);
}

lval *qexpr_tail(lval *a, lval::iter begin) {
    LASSERT_NOT_EMPTY("tail", a, *begin)

    auto v = lval::take(a, begin);
    delete v->pop_first();
    return v;
}

lval *string_tail(lval *a, lval::iter begin) {
    LASSERT_NOT_EMPTY_STRING("tail", a, *begin)

    auto v = lval::take(a, begin);
    v->str = v->str.substr(1);

    return v;
}

lval *tail(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("tail", a, 1)

    auto begin = a->cells.begin();

    LASSERT_TYPE2("tail", a, *begin, lval_type::qexpr, lval_type::string)

    if ((*begin)->type == lval_type::qexpr)
        return qexpr_tail(a, begin);
    else
        return string_tail(a, begin);
}

lval *list(lenv *e, lval *a) {
    a->type = lval_type::qexpr;
    return a;
}

lval *eval(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("eval", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("eval", a, *begin, lval_type::qexpr)

    auto x = lval::take(a, begin);
    return lval::eval_qexpr(e, x);
}

lval *qexpr_join(lval *a) {
    auto x = a->pop_first();
    for (auto expr: a->cells) {
        x->cells.splice(x->cells.end(), expr->cells);
    }

    return x;
}

lval *string_join(lval *a) {
    auto x = a->pop_first();
    for (auto expr: a->cells) {
        x->str += expr->str;
    }

    return x;
}

lval *join(lenv *e, lval *a) {
    auto it = a->cells.begin();
    auto first = *it;
    LASSERT_TYPE2("join", a, first, lval_type::qexpr, lval_type::string)

    for (++it; it != a->cells.end(); ++it) {
        LASSERT_TYPE("join", a, *it, first->type)
    }

    lval *x;
    if (first->type == lval_type::qexpr)
        x = qexpr_join(a);
    else
        x = string_join(a);

    delete a;
    return x;
}

lval *cons(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("cons", a, 2)
    auto it = ++a->cells.begin();

    LASSERT_TYPE("cons", a, *it, lval_type::qexpr)

    auto x = a->pop_first();
    auto v = a->pop_first();
    v->cells.push_front(x);
    delete a;

    return v;
}

lval *len(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("len", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE2("len", a, *begin, lval_type::qexpr, lval_type::string)

    auto x = lval::take(a, begin);
    auto length = new lval(x->type == lval_type::qexpr ? (long)x->cells.size()
                                                       : (long)x->str.size());
    delete x;
    return length;
}

lval *init(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("init", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("init", a, *begin, lval_type::qexpr)
    LASSERT(a, (*begin)->cells.size() != 0, lerr::passed_nil_expr("init"))

    auto v = lval::take(a, begin);
    auto end = v->cells.end();
    end--;
    delete v->pop(end);
    return v;
}

lval *var(lenv *e, lval *a, const string &func) {
    auto begin = a->cells.begin();

    if ((*begin)->type == lval_type::sexpr) {
        *begin = lval::eval_sexpr(e, *begin);
    }

    LASSERT_TYPE2(func, a, *begin, lval_type::symbol, lval_type::qexpr)

    lval *syms;

    if ((*begin)->type == lval_type::qexpr) {
        syms = *begin;
        for (auto cell: syms->cells) {
            LASSERT(a, cell->type == lval_type::symbol,
                    lerr::cant_define_non_sym(func, cell->type))
        }
    } else {
        *begin = lval::qexpr({*begin});
        syms = *begin;
    }

    LASSERT(a, syms->cells.size() == a->cells.size() - 1,
            lerr::cant_define_mismatched_values(func));

    syms = a->pop_first();
    a = lval::eval_cells(e, a);

    if (a->type == lval_type::error) {
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
    return lval::sexpr();
}

lval *def(lenv *e, lval *a) { return var(e, a, "def"); }

lval *put(lenv *e, lval *a) { return var(e, a, "="); }

lval *lambda(lenv *e, lval *a, const string &func) {
    LASSERT_NUM_ARGS(func, a, 2)
    auto begin = a->cells.begin();

    LASSERT_TYPE(func, a, *begin, lval_type::qexpr)
    ++begin;
    LASSERT_TYPE(func, a, *begin, lval_type::qexpr)

    auto syms = a->cells.front();
    for (auto cell: syms->cells) {
        LASSERT(a, cell->type == lval_type::symbol,
                lerr::cant_define_non_sym(func, cell->type))
    }

    auto formals = a->pop_first();
    auto body = a->pop_first();
    delete a;

    if (func == "\\")
        return lval::function(formals, body);
    else
        return lval::macro(formals, body);
}

lval *func_lambda(lenv *e, lval *a) { return lambda(e, a, "\\"); }

lval *macro_lambda(lenv *e, lval *a) { return lambda(e, a, "\\!"); }

lval *load(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("load", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("load", a, *begin, lval_type::string)

    mpc_result_t r;
    if (mpc_parse_contents((*begin)->str.c_str(), lispy::instance()->parser(),
                           &r)) {
        lval *expr = lval::read((mpc_ast_t *)r.output);
        mpc_ast_delete((mpc_ast_t *)r.output);
        while (!expr->cells.empty()) {
            auto x = lval::eval(e, expr->pop_first());
            if (x->type == lval_type::error) {
                cout << *x << endl;
            }

            delete x;
        }

        delete expr;
        delete a;

        return lval::sexpr();
    } else {
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        auto err = error(lerr::could_not_load_library(err_msg));
        free(err_msg);
        delete a;

        return err;
    }
}

lval *print(lenv *e, lval *a) {
    for (auto cell: a->cells) {
        cout << *cell << ' ';
    }

    cout << endl;
    delete a;

    return lval::sexpr();
}

lval *make_error(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("errror", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("error", a, *begin, lval_type::string)

    lval *err = error((*begin)->str);

    delete a;
    return err;
}

lval *read(lenv *e, lval *a) { return read_file(e, a, "<read>"); }

lval *read_file(lenv *e, lval *a, const string &filename) {
    LASSERT_NUM_ARGS("read", a, 1)
    auto begin = a->cells.begin();

    LASSERT_TYPE("read", a, *begin, lval_type::string)

    mpc_result_t r;
    if (mpc_parse(filename.c_str(), (*begin)->str.c_str(),
                  lispy::instance()->parser(), &r)) {
        lval *result = lval::read((mpc_ast_t *)r.output);
        result->type = lval_type::qexpr;

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

lval *show(lenv *e, lval *a) {
    for (auto cell: a->cells) {
        LASSERT_TYPE("show", a, cell, lval_type::string)
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

    return lval::sexpr();
}

namespace repl {

lval *clear(lenv *e, lval *a) {
    LASSERT_NUM_ARGS("clear", a, 0)

    auto lspy = lispy::instance();
    lspy->flags |= LISPY_FLAG_CLEAR_OUTPUT;

    return lval::sexpr();
}

} // namespace repl

} // namespace builtin
