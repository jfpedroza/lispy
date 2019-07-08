#include "lval_error.hpp"

using std::string;

namespace lerr {
    string bad_num() {
        return "Invalid number!";
    }

    string unknown_sym(string symbol) {
        return "Unkown symbol '" + symbol + "'";
    }

    string div_zero() {
        return "Division by zero!";
    }

    string int_mod() {
        return "Module operation can only be applied to integers!";
    }

    string sexpr_not_symbol() {
        return "S-expression Does not start with symbol!";
    }

    string cant_oper_non_num() {
        return "Cannot operate on non-number!";
    }
}

