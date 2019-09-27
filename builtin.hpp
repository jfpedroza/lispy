#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <functional>
#include <string>

struct LValue;
struct lenv;

using lbuiltin = std::function<LValue *(lenv *, LValue *)>;

namespace builtin {

void add_builtins(lenv *env);
void add_builtin_commands(lenv *env);

// Variable functions
LValue *def(lenv *env, LValue *args);
LValue *put(lenv *env, LValue *args);
LValue *func_lambda(lenv *env, LValue *args);
LValue *macro_lambda(lenv *env, LValue *args);

// Operators
lbuiltin ope(const std::string &op);
LValue *handle_op(lenv *env, LValue *args, const std::string &op);
LValue *add(LValue *x, LValue *y);
LValue *substract(LValue *x, LValue *y);
LValue *multiply(LValue *x, LValue *y);
LValue *divide(LValue *x, LValue *y);
LValue *reminder(LValue *x, LValue *y);
LValue *power(LValue *x, LValue *y);
LValue *negate(LValue *x);

// Math functions
LValue *minimum(LValue *x, LValue *y);
LValue *maximum(LValue *x, LValue *y);

// Comparison functions
lbuiltin ordering_op(const std::string &op);
LValue *ord(lenv *env, LValue *args, const std::string &op);
LValue *cmp(lenv *env, LValue *args, const std::string &op);
LValue *equals(lenv *env, LValue *args);
LValue *not_equals(lenv *env, LValue *args);
LValue *if_(lenv *env, LValue *args);

// List functions
LValue *head(lenv *env, LValue *args);
LValue *tail(lenv *env, LValue *args);
LValue *list(lenv *env, LValue *args);
LValue *eval(lenv *env, LValue *args);
LValue *join(lenv *env, LValue *args);
LValue *cons(lenv *env, LValue *args);
LValue *len(lenv *env, LValue *args);
LValue *init(lenv *env, LValue *args);

// String functions
LValue *load(lenv *env, LValue *args);
LValue *print(lenv *env, LValue *args);
LValue *make_error(lenv *env, LValue *args);
LValue *read(lenv *env, LValue *args);
LValue *read_file(lenv *env, LValue *args, const std::string &filename);
LValue *show(lenv *env, LValue *args);

// System functions
LValue *exit(lenv *env, LValue *args);

// REPL commands
namespace repl {
LValue *clear(lenv *env, LValue *args);
LValue *print_env(lenv *env, LValue *args);
LValue *quit(lenv *env, LValue *args);
} // namespace repl
} // namespace builtin

#endif // LISPY_BUILTIN_HPP
