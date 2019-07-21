#include <cstdlib>
#include <unordered_map>
#include <functional>
#include "builtin.hpp"
#include "lval.hpp"
#include "lval_error.hpp"
#include "lenv.hpp"

using std::string;

#define LVAL_OPERATOR_BASE(X, Y, E1, E2, E3, E4) \
switch (X->type) { \
    case lval_type::decimal: \
        switch (Y->type) { \
            case lval_type::decimal: E1; \
            case lval_type::integer: E2; \
            default: return Y; \
        } \
    case lval_type::integer: \
        switch (Y->type) { \
            case lval_type::decimal: E3; \
            case lval_type::integer: E4; \
            default: return Y; \
        } \
        default: return X; \
}

#define LVAL_OPERATOR(OP, X, Y) \
    LVAL_OPERATOR_BASE(X, Y, \
        X->dec OP Y->dec; return X, \
        X->dec OP Y->integ; return X, \
        X->type = lval_type::decimal; X->dec = X->integ; X->dec OP Y->dec; return X, \
        X->integ OP Y->integ; return X \
    )

#define LVAL_BINARY_HANDLER(HANDLER, X, Y) \
    LVAL_OPERATOR_BASE(X, Y, \
        X->dec = HANDLER(X->dec, Y->dec); return X, \
        X->dec = HANDLER(X->dec, Y->integ); return X, \
        X->type = lval_type::decimal; X->dec = HANDLER(X->integ, Y->dec); return X, \
        X->integ = HANDLER(X->integ, Y->integ); return X \
    )

#define LVAL_COMPARISON(OP, X, Y, A, B) \
    comp = [X, Y]() { \
        auto a = X->A; \
        auto b = Y->B; \
        delete X; \
        delete Y; \
        return new lval(a OP b); \
    }; \
 \
   return comp();

#define LVAL_COMP_OPERATOR(OP, X, Y) \
    function<lval*()> comp; \
    LVAL_OPERATOR_BASE(X, Y, \
        LVAL_COMPARISON(OP, X, Y, dec, dec), \
        LVAL_COMPARISON(OP, X, Y, dec, integ), \
        LVAL_COMPARISON(OP, X, Y, integ, dec), \
        LVAL_COMPARISON(OP, X, Y, integ, integ) \
    )

#define LASSERT(args, cond, err) \
  if (!(cond)) { \
      auto msg = error(err); \
      delete args; \
      return msg; \
  }

#define LASSERT_NUM_ARGS(func, args, num) \
  LASSERT(args, args->cells.size() == num, lerr::mismatched_num_args(func, args->cells.size(), num))

#define LASSERT_TYPE(func, args, cell, expected) \
  LASSERT(args, (cell)->type == expected, lerr::passed_incorrect_type(func, (cell)->type, expected))

#define LASSERT_NOT_EMPTY(func, args, cell) \
  LASSERT(args, (cell)->cells.size() != 0, lerr::passed_nil_expr(func))

#define LASSERT_NUMBER(func, args, cell) \
    LASSERT(args, (cell)->type == lval_type::integer || (cell)->type == lval_type::decimal, \
        lerr::passed_incorrect_type(func, (cell)->type, lval_type::number))

namespace builtin {

    using std::unordered_map;
    using std::function;

    auto error = lval::error;

    unordered_map<string, function<lval*(lval*, lval*)>> operator_table = {
        {"+", add},
        {"-", substract},
        {"*", multiply},
        {"/", divide},
        {"%", reminder},
        {"^", power},
        {"min", minimum},
        {"max", maximum}
    };

    void add_builtins(lenv *e) {

        // Variable functions
        e->add_builtin("def", def);
        e->add_builtin("=", put);
        e->add_builtin("\\", lambda);

        // Math functions
        e->add_builtin("+", ope("+"));
        e->add_builtin("-", ope("-"));
        e->add_builtin("*", ope("*"));
        e->add_builtin("/", ope("/"));
        e->add_builtin("%", ope("%"));
        e->add_builtin("^", ope("^"));
        e->add_builtin("min", ope("min"));
        e->add_builtin("max", ope("max"));

        // Comparison functions
        e->add_builtin("==", equals);
        e->add_builtin("!=", not_equals);
        e->add_builtin(">", ordering_op(">"));
        e->add_builtin("<", ordering_op("<"));
        e->add_builtin(">=", ordering_op(">="));
        e->add_builtin("<=", ordering_op("<="));
        e->add_builtin("if", if_);

        // List Functions
        e->add_builtin("head", head);
        e->add_builtin("tail", tail);
        e->add_builtin("list", list);
        e->add_builtin("eval", eval);
        e->add_builtin("join", join);
        e->add_builtin("cons", cons);
        e->add_builtin("len", len);
        e->add_builtin("init", init);

        // Atoms
        lval *True = new lval(true);
        lval *False = new lval(false);
        e->def("true", True);
        e->def("false", False);
        delete True;
        delete False;
    }

    lbuiltin ope(const string &op) {
        using namespace std::placeholders;
        return std::bind(handle_op, _1, _2, op);
    }

    lval* handle_op(lenv *e, lval *a, const string &op) {
        for (auto cell: a->cells) {
            LASSERT_NUMBER(op, a, cell)
        }

        auto x = a->pop_first();
        if (op == "-" && a->cells.empty()) {
            x = negate(x);
        }

        auto op_it = operator_table.find(op);
        auto handler = op_it->second;

        while(!a->cells.empty()) {
            auto y = a->pop_first();
            x = handler(x, y);
            delete y;

            if (x->type == lval_type::error) break;
        }

        delete a;
        return x;
    }

    lval* add(lval *x, lval *y) {
        LVAL_OPERATOR(+=, x, y)
    }

    lval* substract(lval *x, lval *y) {
        LVAL_OPERATOR(-=, x, y)
    }

    lval* multiply(lval *x, lval *y) {
        LVAL_OPERATOR(*=, x, y)
    }

    lval* err_div_zero(lval *x) {
        delete x;
        return error(lerr::div_zero());
    }

    lval* err_int_mod(lval *x) {
        delete x;
        return error(lerr::int_mod());
    }

    lval* divide(lval *x, lval *y) {
        auto e1 = [x, y]() {x->dec /= y->dec; return x;};
        auto e2 = [x, y]() {x->dec /= y->integ; return x;};
        auto e3 = [x, y]() {x->type = lval_type::decimal; x->dec = x->integ; x->dec /= y->dec; return x;};
        auto e4 = [x, y]() {x->integ /= y->integ; return x;};

        LVAL_OPERATOR_BASE(x, y,
                return y->dec == 0 ? err_div_zero(x) : e1(),
                return y->integ == 0 ? err_div_zero(x) : e2(),
                return y->dec == 0 ? err_div_zero(x) : e3(),
                return y->integ == 0 ? err_div_zero(x) : e4()
                )
    }

    lval* reminder(lval *x, lval *y) {
        auto e4 = [x, y]() {x->integ %= y->integ; return x;};

        LVAL_OPERATOR_BASE(x, y,
                return err_int_mod(x),
                return err_int_mod(x),
                return err_int_mod(x),
                return y->integ == 0 ? err_div_zero(x) : e4()
                )
    }

    lval* power(lval *x, lval *y) {
        LVAL_BINARY_HANDLER(pow, x, y)
    }

    lval* negate(lval *x) {
        switch (x->type) {
            case lval_type::decimal:
                x->dec *= -1;
                return x;
            case lval_type::integer:
                x->integ *= -1;
                return x;
            default: return x;
        }
    }

    lval* min_max(std::function<bool(double, double)> comp, lval *x, lval *y) {
        auto e1 = [x, y]() {x->dec = y->dec; return x;};
        auto e2 = [x, y]() {x->dec = y->integ; return x;};
        auto e3 = [x, y]() {x->type = lval_type::decimal; x->dec = y->dec; return x;};
        auto e4 = [x, y]() {x->integ = y->integ; return x;};

        LVAL_OPERATOR_BASE(x, y,
                return comp(x->dec, y->dec) ? x : e1(),
                return comp(x->dec, y->integ) ? x : e2(),
                return comp(x->integ, y->dec) ? x : e3(),
                return comp(x->integ, y->integ) ? x : e4()
                )
    }

    lval* minimum(lval *x, lval *y) {
        return min_max(std::less_equal<double>(), x, y);
    }

    lval* maximum(lval *x, lval *y) {
        return min_max(std::greater_equal<double>(), x, y);
    }

    lbuiltin ordering_op(const string &op) {
        using namespace std::placeholders;
        return std::bind(ord, _1, _2, op);
    }

    lval* ord(lenv *e, lval *a, const std::string &op) {
        LASSERT_NUM_ARGS(op, a, 2)
        auto begin = a->cells.begin();

        LASSERT_NUMBER(op, a, *begin)
        ++begin;
        LASSERT_NUMBER(op, a, *begin)

        auto x = a->pop_first();
        auto y = a->pop_first();
        delete a;

        if (op == "==") { LVAL_COMP_OPERATOR(==, x, y) }
        else if (op == "!=") { LVAL_COMP_OPERATOR(!=, x, y) }
        else if (op == ">") { LVAL_COMP_OPERATOR(>, x, y) }
        else if (op == "<") { LVAL_COMP_OPERATOR(<, x, y) }
        else if (op == ">=") { LVAL_COMP_OPERATOR(>=, x, y) }
        else if (op == "<=") { LVAL_COMP_OPERATOR(<=, x, y) }

        return error("Fatal! Weird operator '" + op + "' in ord");
    }

    lval* cmp(lenv *e, lval *a, const std::string &op) {
        LASSERT_NUM_ARGS(op, a, 2)
        auto begin = a->cells.begin();

        auto x = *begin++;
        auto y = *begin;

        bool result = false;

        if (op == "==") result = *x == *y;
        else if (op == "!=") result = *x != *y;
        delete a;

        return new lval(result);
    }

    lval* equals(lenv *e, lval *a) {
        return cmp(e, a, "==");
    }

    lval* not_equals(lenv *e, lval *a) {
        return cmp(e, a, "!=");
    }

    lval* if_(lenv *e, lval *a) {
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

    lval* head(lenv *e, lval *a) {
        LASSERT_NUM_ARGS("head", a, 1)

        auto begin = a->cells.begin();

        LASSERT_TYPE("head", a, *begin, lval_type::qexpr)
        LASSERT_NOT_EMPTY("head", a, *begin)

        auto v = lval::take(a, begin);
        while (v->cells.size() > 1) {
            delete v->pop(1);
        }

        return v;
    }

    lval* tail(lenv *e, lval *a) {
        LASSERT_NUM_ARGS("tail", a, 1)

        auto begin = a->cells.begin();

        LASSERT_TYPE("tail", a, *begin, lval_type::qexpr)
        LASSERT_NOT_EMPTY("tail", a, *begin)

        auto v = lval::take(a, begin);
        delete v->pop_first();
        return v;
    }

    lval* list(lenv *e, lval *a) {
        a->type = lval_type::qexpr;
        return a;
    }

    lval* eval(lenv *e, lval *a) {
        LASSERT_NUM_ARGS("eval", a, 1)
        auto begin = a->cells.begin();

        LASSERT_TYPE("eval", a, *begin, lval_type::qexpr)

        auto x = lval::take(a, begin);
        return lval::eval_qexpr(e, x);
    }

    lval* join(lenv *e, lval *a) {
        for (auto cell: a->cells) {
            LASSERT_TYPE("join", a, cell, lval_type::qexpr)
        }

        auto x = a->pop_first();
        for (auto expr: a->cells) {
            x->cells.splice(x->cells.end(), expr->cells);
        }

        delete a;
        return x;
    }

    lval* cons(lenv *e, lval *a) {
        LASSERT_NUM_ARGS("cons", a, 2)
        auto it = ++a->cells.begin();

        LASSERT_TYPE("cons", a, *it, lval_type::qexpr)

        auto x = a->pop_first();
        auto v = a->pop_first();
        v->cells.push_front(x);
        delete a;

        return v;
    }

    lval* len(lenv *e, lval *a) {
        LASSERT_NUM_ARGS("len", a, 1)
        auto begin = a->cells.begin();

        LASSERT_TYPE("len", a, *begin, lval_type::qexpr)

        auto x = lval::take(a, begin);
        auto length = new lval((long)x->cells.size());
        delete x;
        return length;
    }

    lval* init(lenv *e, lval *a) {
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

    lval* var(lenv *e, lval *a, const string &func) {
        auto begin = a->cells.begin();

        LASSERT_TYPE(func, a, *begin, lval_type::qexpr)

        auto syms = *begin;
        for (auto cell: syms->cells) {
            LASSERT(a, cell->type == lval_type::symbol, lerr::cant_define_non_sym(func, cell->type))
        }

        LASSERT(a, syms->cells.size() == a->cells.size() - 1, lerr::cant_define_mismatched_values(func));

        auto val_it = ++begin;
        for (auto sym_it = syms->cells.begin(); sym_it != syms->cells.end(); ++sym_it, ++val_it) {
            if (func == "def") {
                e->def((*sym_it)->sym, *val_it);
            } else if (func == "=") {
                e->put((*sym_it)->sym, *val_it);
            }
        }

        delete a;
        return lval::sexpr();
    }

    lval* def(lenv *e, lval *a) {
        return var(e, a, "def");
    }

    lval* put(lenv *e, lval *a) {
        return var(e, a, "=");
    }

    lval* lambda(lenv *e, lval *a) {
        LASSERT_NUM_ARGS("\\", a, 2)
        auto begin = a->cells.begin();

        LASSERT_TYPE("\\", a, *begin, lval_type::qexpr)
        ++begin;
        LASSERT_TYPE("\\", a, *begin, lval_type::qexpr)

        auto syms = a->cells.front();
        for (auto cell: syms->cells) {
            LASSERT(a, cell->type == lval_type::symbol, lerr::cant_define_non_sym("\\", cell->type))
        }

        auto formals = a->pop_first();
        auto body = a->pop_first();
        delete a;

        return new lval(formals, body);
    }
}
