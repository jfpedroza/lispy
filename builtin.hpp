#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <string>

struct lval;

namespace builtin {
    lval* handle(lval *v, const std::string &op);
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

#endif // LISPY_BUILTIN_HPP
