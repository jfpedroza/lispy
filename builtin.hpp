#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <string>
#include <functional>

struct lval;
struct lenv;

namespace builtin {
    lval* handle_op(lenv *env, lval *args, const std::string &op);
    std::function<lval*(lenv*, lval*)> ope(lenv *env, lval *args, const std::string &op);

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
    lval* head(lenv *env, lval *args);
    lval* tail(lenv *env, lval *args);
    lval* list(lenv *env, lval *args);
    lval* eval(lenv *env, lval *args);
    lval* join(lenv *env, lval *args);
    lval* cons(lenv *env, lval *args);
    lval* len(lenv *env, lval *args);
    lval* init(lenv *env, lval *args);
}

#endif // LISPY_BUILTIN_HPP
