#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include "mpc.h"
#include "lval_error.hpp"
#include "parsing.hpp"

using std::string;
using std::vector;
using std::list;
using std::cout;
using std::endl;
using std::ostream;

enum class lval_type {
    integer,
    decimal,
    symbol,
    sexpr,
    error
};

vector<string> content_skip_strings = {"(", ")"};
bool should_skip_child(mpc_ast_t *child) {
    auto end = content_skip_strings.end();
    auto it = std::find(content_skip_strings.begin(), end, child->contents);
    return it != end;
}

struct lval;

namespace builtin {
    lval* handle(lval *v, string op);
    lval* add(lval *x, lval *y);
    lval* substract(lval *x, lval *y);
    lval* multiply(lval *x, lval *y);
    lval* divide(lval *x, lval *y);
    lval* reminder(lval *x, lval *y);
    lval* power(lval *x, lval *y);
    lval* negate(lval *x);
    lval* minimum(lval *x, lval *y);
    lval* maximum(lval *x, lval *y);
}

struct lval {
    lval_type type;
    long integ;
    double dec;
    string err;
    string sym;
    list<lval*> cells;

    using iter = list<lval*>::iterator;

    lval(lval_type type) {
        this->type = type;
    }

    lval(long num) {
        this->type = lval_type::integer;
        this->integ = num;
    }

    lval(double num) {
        this->type = lval_type::decimal;
        this->dec = num;
    }

    explicit lval(string sym) {
        this->type = lval_type::symbol;
        this->sym = sym;
    }

    static lval* error(string err) {
        auto val = new lval(lval_type::error);
        val->err = err;
        return val;
    }

    static lval* sexpr() {
        auto val = new lval(lval_type::sexpr);
        return val;
    }

    ~lval() {
        for(auto cell: cells) {
            delete cell;
        }
    }

    lval* pop(const iter &it) {
        auto x = *it;
        cells.erase(it);
        return x;
    }

    lval* pop(uint i) {
        auto it = cells.begin();
        for (uint pos = 0; pos < i; pos++, ++it) {}

        return pop(it);
    }

    lval* pop_first() {
        return pop(cells.begin());
    }

    static lval* take(lval *v, const iter &it) {
        auto x = v->pop(it);
        delete v;
        return x;
    }

    static lval* take(lval *v, uint i) {
        auto x = v->pop(i);
        delete v;
        return x;
    }

    static lval* take_first(lval *v) {
        return take(v, v->cells.begin());
    }

    static lval* read_integer(mpc_ast_t *t) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ?
            new lval(x) : error(lerr::bad_num());
    }

    static lval* read_decimal(mpc_ast_t *t) {
        errno = 0;
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ?
            new lval(x) : error(lerr::bad_num());
    }

    static lval* read(mpc_ast_t *t) {
        if (strstr(t->tag, "integer")) return read_integer(t);
        if (strstr(t->tag, "decimal")) return read_decimal(t);
        if (strstr(t->tag, "symbol")) return new lval(t->contents);

        lval *x = nullptr;
        if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
            x = sexpr();
        }

        for (int i = 0; i < t->children_num; i++) {
            if (should_skip_child(t->children[i])) continue;
            if (strcmp(t->children[i]->tag,  "regex") == 0) continue;

            x->cells.push_back(read(t->children[i]));
        }

        return x;
    }

    static lval* eval(lval *v) {
        if (v->type == lval_type::sexpr) return eval_sexpr(v);

        return v;
    }

    static lval* eval_sexpr(lval *v) {
        std::transform(v->cells.begin(), v->cells.end(), v->cells.begin(), eval);

        for (auto it = v->cells.begin(); it != v->cells.end(); ++it) {
            if ((*it)->type == lval_type::error) return take(v, it);
        }

        if (v->cells.empty()) return v;

        if (v->cells.size() == 1) return take_first(v);

        auto f = v->pop_first();
        if (f->type != lval_type::symbol) {
            delete f;
            delete v;
            return error(lerr::sexpr_not_symbol());
        }

        auto result = builtin::handle(v, f->sym);
        delete f;

        return result;
    }

    friend ostream& operator<<(ostream &os, const lval &value);

    ostream& print_expr(ostream &os, char open, char close) const {
        os << open;

        for (auto it = cells.begin(); it != cells.end();) {
            os << **it;

            if (++it != cells.end()) os << ' ';
        }

        return os << close;
    }
};

ostream& operator<<(ostream &os, const lval &value) {
    switch (value.type) {
        case lval_type::integer:
            return os << value.integ;

        case lval_type::decimal:
            return os << value.dec;

        case lval_type::symbol:
            return os << value.sym;

        case lval_type::error:
            return os << "Error: " << value.err;

        case lval_type::sexpr:
            return value.print_expr(os, '(', ')');
    }

    return os;
}

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

namespace builtin {

    using std::unordered_map;
    using std::function;

    auto error = lval::error;

    unordered_map<string, function<lval*(lval*, lval*)>> symbol_table = {
        {"+", add}, {"add", add},
        {"-", substract}, {"sub", substract},
        {"*", multiply}, {"mul", multiply},
        {"/", divide}, {"div", divide},
        {"%", reminder}, {"rem", reminder},
        {"^", power}, {"pow", power},
        {"min", minimum},
        {"max", maximum},
    };

    lval* handle(lval *v, string op) {
        for (auto cell: v->cells) {
            if (cell->type != lval_type::integer && cell->type != lval_type::decimal) {
                delete v;
                return error(lerr::cant_oper_non_num());
            }
        }

        auto x = v->pop_first();
        if (op == "-" && v->cells.empty()) {
            x = negate(x);
        }

        auto symbol_it = symbol_table.find(op);
        if (symbol_it == symbol_table.end()) {
            return error(lerr::unknown_sym(op));
        }

        auto handler = symbol_it->second;

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
}

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

