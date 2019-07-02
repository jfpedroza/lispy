#include "lval_error.hpp"

namespace lerr {
    std::string bad_num() {
        return "Invalid number!";
    }

    std::string unknown_sym(std::string symbol) {
        return "Unkown symbol '" + symbol + "'";
    }

    std::string div_zero() {
        return "Division by zero!";
    }

    std::string int_mod() {
        return "Module operation can only be applied to integers!";
    }

    std::string sexpr_not_symbol() {
        return "S-expression Does not start with symbol!";
    }

    std::string cant_oper_non_num() {
        return "Cannot operate on non-number!";
    }
}

