#include <string>
#include "lenv.hpp"
#include "lval.hpp"
#include "lval_error.hpp"

using std::string;
auto error = lval::error;

lenv::lenv() {
    this->parent = nullptr;
}

lenv::lenv(const lenv &other) {
    this->parent = other.parent;
    this->symbols = table_type(other.symbols);
    for (auto it = this->symbols.begin(); it != this->symbols.end(); ++it) {
        it->second = new lval(it->second);
    }
}

lenv::lenv(const lenv *const other): lenv(*other) {}

lenv::~lenv() {
    for (auto entry: this->symbols) {
        delete entry.second;
    }
}

lval* lenv::get(const string &sym) const {
    auto it = symbols.find(sym);
    if (it != symbols.end()) {
        return new lval(it->second);
    }

    if (parent) {
        return parent->get(sym);
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

void lenv::def(const string &sym, const lval *const v) {
    auto e = this;
    while (e->parent) e = e->parent;

    e->put(sym, v);
}

void lenv::add_builtin(const string &name, lbuiltin func) {
    symbols.insert(std::make_pair(name, new lval(func)));
}
