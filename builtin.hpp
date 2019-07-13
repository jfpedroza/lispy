#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <string>

struct lval;

namespace builtin {
    lval* handle(lval *v, const std::string &func);
    lval* handle_op(lval *v, const std::string &op);

    // Operators
    lval* add(lval *x, lval *y);
    lval* substract(lval *x, lval *y);
    lval* multiply(lval *x, lval *y);
    lval* divide(lval *x, lval *y);
    lval* reminder(lval *x, lval *y);
    lval* power(lval *x, lval *y);
    lval* negate(lval *x);

    // Math functions
    lval* minimum(lval *x, lval *y);
    lval* maximum(lval *x, lval *y);

    // Q-Expressions
    lval* head(lval *v);
    lval* tail(lval *v);
    lval* list(lval *v);
    lval* eval(lval *v);
    lval* join(lval *v);
}

#endif // LISPY_BUILTIN_HPP
