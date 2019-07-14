#include <sstream>
#include "lval_error.hpp"

using std::string;
using std::stringstream;

namespace lerr {
    string bad_num() {
        return "Invalid number!";
    }

    string unknown_sym(const string &symbol) {
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

    string mismatched_num_args(const string &func, size_t got, size_t expected) {
        stringstream ss;
        ss << "Function '" << func << "' passed incorrect number of arguments. Got " << got << ", Expected " << expected << ".";
        return ss.str();
    }

    string passed_incorrect_types(const string &func) {
        return "Function '" + func + "' passed incorrect types!";
    }

    string passed_nil_expr(const string &func) {
        return "Function '" + func + "' passed {}!";
    }
}

