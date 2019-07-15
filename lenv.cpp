#include <string>
#include "lenv.hpp"
#include "lval.hpp"
#include "lval_error.hpp"

using std::string;
auto error = lval::error;

lenv::~lenv() {
    for (auto entry: this->symbols) {
        delete entry.second;
    }
}

lval* lenv::get(const string &sym) const {
    auto it = symbols.find(sym);
    if (it != symbols.end()) {
        return it->second;
    }

    return error(lerr::unknown_sym(sym));
}

void lenv::put(const string &sym, const lval *const v) {
    auto it = symbols.find(sym);
    if (it != symbols.end()) {
        delete it->second;
        it->second = new lval(v);
    } else {
        symbols.insert(std::make_pair(sym, new lval(v)));
    }
}
