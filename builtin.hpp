#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <functional>
#include <string>

struct lval;
struct lenv;

using lbuiltin = std::function<lval *(lenv *, lval *)>;

namespace builtin {

void add_builtins(lenv *env);

// Variable functions
lval *def(lenv *env, lval *args);
lval *put(lenv *env, lval *args);
lval *func_lambda(lenv *env, lval *args);
lval *macro_lambda(lenv *env, lval *args);

// Operators
lbuiltin ope(const std::string &op);
lval *handle_op(lenv *env, lval *args, const std::string &op);
lval *add(lval *x, lval *y);
lval *substract(lval *x, lval *y);
lval *multiply(lval *x, lval *y);
lval *divide(lval *x, lval *y);
lval *reminder(lval *x, lval *y);
lval *power(lval *x, lval *y);
lval *negate(lval *x);

// Math functions
lval *minimum(lval *x, lval *y);
lval *maximum(lval *x, lval *y);

// Comparison functions
lbuiltin ordering_op(const std::string &op);
lval *ord(lenv *env, lval *args, const std::string &op);
lval *cmp(lenv *env, lval *args, const std::string &op);
lval *equals(lenv *env, lval *args);
lval *not_equals(lenv *env, lval *args);
lval *if_(lenv *env, lval *args);

// List functions
lval *head(lenv *env, lval *args);
lval *tail(lenv *env, lval *args);
lval *list(lenv *env, lval *args);
lval *eval(lenv *env, lval *args);
lval *join(lenv *env, lval *args);
lval *cons(lenv *env, lval *args);
lval *len(lenv *env, lval *args);
lval *init(lenv *env, lval *args);

// String funtions
lval *load(lenv *env, lval *args);
lval *print(lenv *env, lval *args);
lval *make_error(lenv *env, lval *args);
lval *read(lenv *env, lval *args);
lval *read_file(lenv *env, lval *args, const std::string &filename);
lval *show(lenv *env, lval *args);
} // namespace builtin

#endif // LISPY_BUILTIN_HPP
