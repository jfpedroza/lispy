#ifndef LVAL_ERROR_HPP
#define LVAL_ERROR_HPP

#include <string>
#include "lval.hpp"

namespace lerr {
    std::string bad_num();
    std::string unknown_sym(const std::string &symbol);
    std::string div_zero();
    std::string int_mod();
    std::string sexpr_not_function(lval_type got);
    std::string mismatched_num_args(const std::string &func, size_t got, size_t expected);
    std::string too_many_args(size_t got, size_t expected);
    std::string passed_incorrect_type(const std::string &func, lval_type got, lval_type expected);
    std::string passed_nil_expr(const std::string &func);
    std::string cant_define_non_sym(const std::string &func, lval_type got);
    std::string cant_define_mismatched_values(const std::string &func);
}

#endif // LVAL_ERROR_HPP
