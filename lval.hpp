#ifndef LVAL_HPP
#define LVAL_HPP

#include <iostream>
#include <string>
#include <list>
#include <functional>
#include "mpc.h"

enum class lval_type {
    integer,
    decimal,
    symbol,
    func,
    sexpr,
    qexpr,
    error
};

struct lval;
struct lenv;

using lbuiltin = std::function<lval*(lenv*, lval*)>;

struct lval {
    lval_type type;
    long integ;
    double dec;
    std::string err;
    std::string sym;
    lbuiltin fun;

    using cell_type = std::list<lval*>;

    cell_type cells;

    using iter = cell_type::iterator;

    lval(lval_type type);

    lval(long num);

    lval(double num);

    explicit lval(std::string sym);

    lval(lbuiltin fun);

    lval(const lval &other);

    lval(const lval *const other);

    static lval* error(std::string err);

    static lval* sexpr();

    static lval* qexpr();

    ~lval();

    lval* pop(const iter &it);

    lval* pop(size_t i);

    lval* pop_first();

    static lval* take(lval *v, const iter &it);

    static lval* take(lval *v, size_t i);

    static lval* take_first(lval *v);

    static lval* read_integer(mpc_ast_t *t);

    static lval* read_decimal(mpc_ast_t *t);

    static lval* read(mpc_ast_t *t);

    static lval* eval(lenv *e, lval *v);

    static lval* eval_sexpr(lenv *e, lval *v);

    friend std::ostream& operator<<(std::ostream &os, const lval &value);

    std::ostream& print_expr(std::ostream &os, char open, char close) const;
};

#endif // LVAL_HPP
