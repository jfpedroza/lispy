#ifndef LISPY_BUILTIN_HPP
#define LISPY_BUILTIN_HPP

#include <functional>
#include <string>

struct LValue;
struct LEnv;

using lbuiltin = std::function<LValue *(LEnv *, LValue *)>;

namespace builtin {

void add_builtins(LEnv *env);
void add_builtin_commands(LEnv *env);

// Variable functions
LValue *def(LEnv *env, LValue *args);
LValue *put(LEnv *env, LValue *args);
LValue *func_lambda(LEnv *env, LValue *args);
LValue *macro_lambda(LEnv *env, LValue *args);

// Operators
lbuiltin ope(const std::string &op);
LValue *handle_op(LEnv *env, LValue *args, const std::string &op);
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
LValue *ord(LEnv *env, LValue *args, const std::string &op);
LValue *cmp(LEnv *env, LValue *args, const std::string &op);
LValue *equals(LEnv *env, LValue *args);
LValue *not_equals(LEnv *env, LValue *args);
LValue *if_(LEnv *env, LValue *args);

// List functions
LValue *head(LEnv *env, LValue *args);
LValue *tail(LEnv *env, LValue *args);
LValue *list(LEnv *env, LValue *args);
LValue *eval(LEnv *env, LValue *args);
LValue *join(LEnv *env, LValue *args);
LValue *cons(LEnv *env, LValue *args);
LValue *len(LEnv *env, LValue *args);
LValue *init(LEnv *env, LValue *args);

// String functions
LValue *load(LEnv *env, LValue *args);
LValue *print(LEnv *env, LValue *args);
LValue *make_error(LEnv *env, LValue *args);
LValue *read(LEnv *env, LValue *args);
LValue *read_file(LEnv *env, LValue *args, const std::string &filename);
LValue *show(LEnv *env, LValue *args);

// System functions
LValue *exit(LEnv *env, LValue *args);

// REPL commands
namespace repl {
LValue *clear(LEnv *env, LValue *args);
LValue *print_env(LEnv *env, LValue *args);
LValue *quit(LEnv *env, LValue *args);
} // namespace repl
} // namespace builtin

#endif // LISPY_BUILTIN_HPP
