#include "lval_error.hpp"
#include <sstream>

using std::string;
using std::stringstream;

namespace lerr {
string bad_num() { return "Invalid number!"; }

string unknown_sym(const string &symbol) {
    return "Unbound symbol '" + symbol + "'!";
}

string div_zero() { return "Division by zero!"; }

string int_mod() { return "Module operation can only be applied to integers!"; }

string sexpr_not_function(lval_type got) {
    stringstream ss;
    ss << "S-expression does not start with function!. Got " << got << ".";
    return ss.str();
}

string mismatched_num_args(const string &func, size_t got, size_t expected) {
    stringstream ss;
    ss << "Function '" << func << "' passed incorrect number of arguments. Got "
       << got << ", Expected " << expected << ".";
    return ss.str();
}

string too_many_args(size_t got, size_t expected) {
    stringstream ss;
    ss << "Function passed too many arguments. Got " << got << ", Expected "
       << expected << ".";
    return ss.str();
}

string passed_incorrect_type(const string &func, lval_type got,
                             lval_type expected) {
    stringstream ss;
    ss << "Function '" << func << "' passed incorrect type. Got " << got
       << ", Expected " << expected << ".";
    return ss.str();
}

string passed_incorrect_type(const string &func, lval_type got,
                             std::initializer_list<lval_type> expected) {
    stringstream ss;
    ss << "Function '" << func << "' passed incorrect type. Got " << got
       << ", Expected one of ";

    for (auto it = expected.begin(); it != expected.end();) {
        ss << *it;
        if (++it == expected.end()) {
            ss << '.';
        } else {
            ss << ", ";
        }
    }

    return ss.str();
}

string passed_nil_expr(const string &func) {
    return "Function '" + func + "' passed {}!";
}

string passed_empty_string(const string &func) {
    return "Function '" + func + "' passed empty string!";
}

string cant_define_non_sym(const string &func, lval_type got) {
    stringstream ss;
    ss << "Function '" << func << "' cannot define non-symbol!. Got " << got
       << ".";
    return ss.str();
}

string cant_define_mismatched_values(const string &func) {
    return "Function '" + func +
           "' cannot define incorrect number of values to symbols";
}

string function_format_invalid() {
    return "Function format invalid. Symbol '&' not followed by single symbol.";
}

string could_not_load_library(const string &msg) {
    return "Cound not load library " + msg;
}
} // namespace lerr
