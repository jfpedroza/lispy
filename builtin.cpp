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

    lval* handle(lval *v, const string &func) {
        auto symbol_it = symbol_table.find(func);
        if (symbol_it != symbol_table.end()) {
            auto handler = symbol_it->second;
            return handler(v);
        } else {
            return handle_op(v, func);
        }
    }

    lval* handle_op(lval *v, const string &op) {
        for (auto cell: v->cells) {
            LASSERT(v, cell->type == lval_type::integer || cell->type == lval_type::decimal, lerr::cant_oper_non_num())
        }

        auto x = v->pop_first();
        if (op == "-" && v->cells.empty()) {
            x = negate(x);
        }

        auto op_it = operator_table.find(op);
        if (op_it == operator_table.end()) {
            return error(lerr::unknown_sym(op));
        }

        auto handler = op_it->second;

        while(!v->cells.empty()) {
            auto y = v->pop_first();
            x = handler(x, y);
            delete y;

            if (x->type == lval_type::error) break;
        }

        delete v;
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

    lval* head(lval *v) {
        LASSERT(v, v->cells.size() == 1, lerr::too_many_args("head"))

        auto begin = v->cells.begin();

        LASSERT(v, (*begin)->type == lval_type::qexpr, lerr::passed_incorrect_types("head"))
        LASSERT(v, (*begin)->cells.size() != 0, lerr::passed_nil_expr("head"))

        auto qexpr = lval::take(v, begin);
        while (qexpr->cells.size() > 1) {
            delete qexpr->pop(1);
        }

        return qexpr;
    }

    lval* tail(lval *v) {
        LASSERT(v, v->cells.size() == 1, lerr::too_many_args("tail"))

        auto begin = v->cells.begin();

        LASSERT(v, (*begin)->type == lval_type::qexpr, lerr::passed_incorrect_types("tail"))
        LASSERT(v, (*begin)->cells.size() != 0, lerr::passed_nil_expr("tail"))

        auto qexpr = lval::take(v, begin);
        delete qexpr->pop_first();
        return qexpr;
    }

    lval* list(lval *v) {
        v->type = lval_type::qexpr;
        return v;
    }

    lval* eval(lval *v) {
        LASSERT(v, v->cells.size() == 1, lerr::too_many_args("eval"))
        auto begin = v->cells.begin();

        LASSERT(v, (*begin)->type == lval_type::qexpr, lerr::passed_incorrect_types("eval"))

        auto qexpr = lval::take(v, begin);
        qexpr->type = lval_type::sexpr;
        return lval::eval(qexpr);
    }

    lval* join(lval *v) {
        for (auto cell: v->cells) {
            LASSERT(v, cell->type == lval_type::qexpr, lerr::passed_incorrect_types("join"))
        }

        auto x = v->pop_first();
        for (auto expr: v->cells) {
            x->cells.splice(x->cells.end(), expr->cells);
        }

        delete v;
        return x;
    }
}
