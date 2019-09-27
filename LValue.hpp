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

struct LValue {
    lval_type type;

    long integ;
    double dec;
    bool boolean;
    std::string err;
    std::string sym;
    std::string str;

    lbuiltin builtin;
    lenv *env;
    LValue *formals;
    LValue *body;

    using cell_type = std::list<LValue *>;

    cell_type cells;

    using iter = cell_type::iterator;

    explicit LValue(lval_type type);

    explicit LValue(long num);

    explicit LValue(double num);

    explicit LValue(bool boolean);

    explicit LValue(std::string str);

    LValue(const LValue &other);

    explicit LValue(const LValue *const other);

    static LValue *symbol(std::string err);

    static LValue *cname(std::string err);

    static LValue *error(std::string err);

    static LValue *function(lbuiltin fun);

    static LValue *function(LValue *formals, LValue *body);

    static LValue *macro(lbuiltin fun);

    static LValue *macro(LValue *formals, LValue *body);

    static LValue *command(lbuiltin fun);

    static LValue *sexpr();

    static LValue *sexpr(std::initializer_list<LValue *> cells);

    static LValue *qexpr();

    static LValue *qexpr(std::initializer_list<LValue *> cells);

    ~LValue();

    bool is_number() const;
    double get_number() const;

    LValue *pop(const iter &it);

    LValue *pop(size_t i);

    LValue *pop_first();

    LValue *call(lenv *e, LValue *a);

    static LValue *take(LValue *v, const iter &it);

    static LValue *take(LValue *v, size_t i);

    static LValue *take_first(LValue *v);

    static LValue *read_integer(mpc_ast_t *t);

    static LValue *read_decimal(mpc_ast_t *t);

    static LValue *read_string(mpc_ast_t *t);

    static LValue *read(mpc_ast_t *t);

    static LValue *eval(lenv *e, LValue *v);

    static LValue *eval_sexpr(lenv *e, LValue *v);

    static LValue *eval_qexpr(lenv *e, LValue *v);

    static LValue *eval_cells(lenv *e, LValue *v);

    friend std::ostream &operator<<(std::ostream &os, const LValue &value);

    std::ostream &print_expr(std::ostream &os, char open, char close) const;
    std::ostream &print_str(std::ostream &os) const;

    bool operator==(const LValue &other) const;
    bool operator!=(const LValue &other) const;
};

#endif // LVAL_HPP
