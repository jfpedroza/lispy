#ifndef LVAL_ERROR_HPP
#define LVAL_ERROR_HPP

#include <string>

namespace lerr {
    std::string bad_num();
    std::string unknown_sym(const std::string &symbol);
    std::string div_zero();
    std::string int_mod();
    std::string sexpr_not_function();
    std::string cant_oper_non_num();
    std::string mismatched_num_args(const std::string &func, size_t got, size_t expected);
    std::string passed_incorrect_types(const std::string &func);
    std::string passed_nil_expr(const std::string &func);
}

#endif // LVAL_ERROR_HPP
