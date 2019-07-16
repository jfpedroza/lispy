#include <sstream>
#include "lval_error.hpp"

using std::string;
using std::stringstream;

namespace lerr {
    string bad_num() {
        return "Invalid number!";
    }

    string unknown_sym(const string &symbol) {
        return "Unbound symbol '" + symbol + "'!";
    }

    string div_zero() {
        return "Division by zero!";
    }

    string int_mod() {
        return "Module operation can only be applied to integers!";
    }

    string sexpr_not_function() {
        return "S-expression does not start with function!";
    }

    string mismatched_num_args(const string &func, size_t got, size_t expected) {
        stringstream ss;
        ss << "Function '" << func << "' passed incorrect number of arguments. Got " << got << ", Expected " << expected << ".";
        return ss.str();
    }

    string passed_incorrect_type(const string &func, lval_type got, lval_type expected) {
        stringstream ss;
        ss << "Function '" << func << "' passed incorrect type. Got " << got << ", Expected " << expected << ".";
        return ss.str();
    }

    string passed_nil_expr(const string &func) {
        return "Function '" + func + "' passed {}!";
    }

    string cant_define_non_sym(const string &func) {
        return "Function '" + func + "' cannot define non-symbol!";
    }

    string cant_define_mismatched_values(const string &func) {
        return "Function '" + func + "' cannot define incorrect number of values to symbols";
    }
}

