#ifndef LVAL_HPP
#define LVAL_HPP

#include <iostream>
#include <list>
#include <string>
#include "builtin.hpp"
#include "mpc.h"

enum class lval_type {
    integer,
    decimal,
    number,
    boolean,
    symbol,
    cname,
    string,
    func,
    macro,
    command,
    sexpr,
    qexpr,
    error
};

std::ostream &operator<<(std::ostream &os, const lval_type &type);

struct lenv;

struct lval {
    lval_type type;

    long integ;
    double dec;
    bool boolean;
    std::string err;
    std::string sym;
    std::string str;

    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;

    using cell_type = std::list<lval *>;

    cell_type cells;

    using iter = cell_type::iterator;

    explicit lval(lval_type type);

    explicit lval(long num);

    explicit lval(double num);

    explicit lval(bool boolean);

    explicit lval(std::string str);

    lval(const lval &other);

    explicit lval(const lval *const other);

    static lval *symbol(std::string err);

    static lval *cname(std::string err);

    static lval *error(std::string err);

    static lval *function(lbuiltin fun);

    static lval *function(lval *formals, lval *body);

    static lval *macro(lbuiltin fun);

    static lval *macro(lval *formals, lval *body);

    static lval *command(lbuiltin fun);

    static lval *sexpr();

    static lval *sexpr(std::initializer_list<lval *> cells);

    static lval *qexpr();

    static lval *qexpr(std::initializer_list<lval *> cells);

    ~lval();

    bool is_number() const;
    double get_number() const;

    lval *pop(const iter &it);

    lval *pop(size_t i);

    lval *pop_first();

    lval *call(lenv *e, lval *a);

    static lval *take(lval *v, const iter &it);

    static lval *take(lval *v, size_t i);

    static lval *take_first(lval *v);

    static lval *read_integer(mpc_ast_t *t);

    static lval *read_decimal(mpc_ast_t *t);

    static lval *read_string(mpc_ast_t *t);

    static lval *read(mpc_ast_t *t);

    static lval *eval(lenv *e, lval *v);

    static lval *eval_sexpr(lenv *e, lval *v);

    static lval *eval_qexpr(lenv *e, lval *v);

    static lval *eval_cells(lenv *e, lval *v);

    friend std::ostream &operator<<(std::ostream &os, const lval &value);

    std::ostream &print_expr(std::ostream &os, char open, char close) const;
    std::ostream &print_str(std::ostream &os) const;

    bool operator==(const lval &other) const;
    bool operator!=(const lval &other) const;
};

#endif // LVAL_HPP
