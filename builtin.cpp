#include <cstdlib>
#include <unordered_map>
#include <functional>
#include "builtin.hpp"
#include "lval.hpp"
#include "lval_error.hpp"

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

#define LVAL_OPERATOR(OP) \
    LVAL_OPERATOR_BASE(x, y, \
            x->dec OP y->dec; return x, \
            x->dec OP y->integ; return x, \
            x->type = lval_type::decimal; x->dec = x->integ; x->dec OP y->dec; return x, \
            x->integ OP y->integ; return x \
        )

#define LVAL_BINARY_HANDLER(HANDLER) \
    LVAL_OPERATOR_BASE(x, y, \
            x->dec = HANDLER(x->dec, y->dec); return x, \
            x->dec = HANDLER(x->dec, y->integ); return x, \
            x->type = lval_type::decimal; x->dec = HANDLER(x->integ, y->dec); return x, \
            x->integ = HANDLER(x->integ, y->integ); return x \
            )

#define LASSERT(args, cond, err) \
  if (!(cond)) { delete args; return error(err); }

#define LASSERT_NUM_ARGS(func, args, num) \
  LASSERT(args, args->cells.size() == num, lerr::mismatched_num_args(func, args->cells.size(), num))

namespace builtin {

    using std::unordered_map;
    using std::function;

    auto error = lval::error;

    unordered_map<string, function<lval*(lval*)>> symbol_table = {
        {"head", head},
        {"tail", tail},
        {"list", list},
        {"eval", eval},
        {"join", join}
    };

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

    lval* handle(lval *a, const string &func) {
        auto symbol_it = symbol_table.find(func);
        if (symbol_it != symbol_table.end()) {
            auto handler = symbol_it->second;
            return handler(a);
        } else {
            return handle_op(a, func);
        }
    }

    lval* handle_op(lval *a, const string &op) {
        for (auto cell: a->cells) {
            LASSERT(a, cell->type == lval_type::integer || cell->type == lval_type::decimal, lerr::cant_oper_non_num())
        }

        auto x = a->pop_first();
        if (op == "-" && a->cells.empty()) {
            x = negate(x);
        }

        auto op_it = operator_table.find(op);
        if (op_it == operator_table.end()) {
            return error(lerr::unknown_sym(op));
        }

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
        LVAL_OPERATOR(+=)
    }

    lval* substract(lval *x, lval *y) {
        LVAL_OPERATOR(-=)
    }

    lval* multiply(lval *x, lval *y) {
        LVAL_OPERATOR(*=)
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
        LVAL_BINARY_HANDLER(pow)
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

    lval* head(lval *a) {
        LASSERT_NUM_ARGS("head", a, 1)

        auto begin = a->cells.begin();

        LASSERT(a, (*begin)->type == lval_type::qexpr, lerr::passed_incorrect_types("head"))
        LASSERT(a, (*begin)->cells.size() != 0, lerr::passed_nil_expr("head"))

        auto v = lval::take(a, begin);
        while (v->cells.size() > 1) {
            delete v->pop(1);
        }

        return v;
    }

    lval* tail(lval *a) {
        LASSERT_NUM_ARGS("tail", a, 1)

        auto begin = a->cells.begin();

        LASSERT(a, (*begin)->type == lval_type::qexpr, lerr::passed_incorrect_types("tail"))
        LASSERT(a, (*begin)->cells.size() != 0, lerr::passed_nil_expr("tail"))

        auto v = lval::take(a, begin);
        delete v->pop_first();
        return v;
    }

    lval* list(lval *a) {
        a->type = lval_type::qexpr;
        return a;
    }

    lval* eval(lval *a) {
        LASSERT_NUM_ARGS("eval", a, 1)
        auto begin = a->cells.begin();

        LASSERT(a, (*begin)->type == lval_type::qexpr, lerr::passed_incorrect_types("eval"))

        auto x = lval::take(a, begin);
        x->type = lval_type::sexpr;
        return lval::eval(x);
    }

    lval* join(lval *a) {
        for (auto cell: a->cells) {
            LASSERT(a, cell->type == lval_type::qexpr, lerr::passed_incorrect_types("join"))
        }

        auto x = a->pop_first();
        for (auto expr: a->cells) {
            x->cells.splice(x->cells.end(), expr->cells);
        }

        delete a;
        return x;
    }
}
