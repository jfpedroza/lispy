#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <string>

struct lval;

namespace builtin {
    lval* handle(lval *args, const std::string &func);
    lval* handle_op(lval *args, const std::string &op);

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
    lval* head(lval *args);
    lval* tail(lval *args);
    lval* list(lval *args);
    lval* eval(lval *args);
    lval* join(lval *args);
}

#endif // LISPY_BUILTIN_HPP
