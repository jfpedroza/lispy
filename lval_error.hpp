#ifndef LVAL_ERROR_HPP
#define LVAL_ERROR_HPP

#include <string>

namespace lerr {
    std::string bad_num();
    std::string unknown_sym(std::string symbol);
    std::string div_zero();
    std::string int_mod();
    std::string sexpr_not_symbol();
    std::string cant_oper_non_num();
}

#endif // LVAL_ERROR_HPP
