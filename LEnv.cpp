#include "LEnv.hpp"
#include <algorithm>
#include "LValue.hpp"
#include "lval_error.hpp"

using std::string;
using std::vector;
auto error = LValue::error;

LEnv::LEnv() { this->parent = nullptr; }

LEnv::LEnv(const LEnv &other): symbols(table_type(other.symbols)) {
    this->parent = other.parent;
    for (auto it = this->symbols.begin(); it != this->symbols.end(); ++it) {
        it->second = new LValue(it->second);
    }
}

LEnv::LEnv(const LEnv *const other): LEnv(*other) {}

LEnv::~LEnv() {
    for (auto entry: this->symbols) {
        delete entry.second;
    }
}

vector<string> LEnv::keys() const {
    vector<string> keys;
    keys.reserve(symbols.size());
    std::transform(symbols.begin(), symbols.end(), std::back_inserter(keys),
                   [](auto it) { return it.first; });
    return keys;
}

vector<const string *> LEnv::keys(const string &prefix) const {
    vector<const string *> keys;
    keys.reserve(symbols.size());

    for (auto it = symbols.begin(); it != symbols.end(); ++it) {
        auto &sym = it->first;
        if (prefix.size() <= sym.size() &&
            std::equal(prefix.begin(), prefix.end(), sym.begin())) {
            keys.push_back(&sym);
        }
    }

    return keys;
}

LValue *LEnv::get(const string &sym) const {
    auto it = symbols.find(sym);
    if (it != symbols.end()) {
        return new LValue(it->second);
    }

    if (parent) {
        return parent->get(sym);
    }

    return error(lerr::unknown_sym(sym));
}

void LEnv::put(const string &sym, const LValue *const v) {
    auto it = symbols.find(sym);
    if (it != symbols.end()) {
        delete it->second;
        it->second = new LValue(v);
    } else {
        symbols.insert(std::make_pair(sym, new LValue(v)));
    }
}

void LEnv::def(const string &sym, const LValue *const v) {
    auto e = this;
    while (e->parent) e = e->parent;

    e->put(sym, v);
}

void LEnv::add_builtin_function(const string &name, lbuiltin func) {
    symbols.insert(std::make_pair(name, LValue::function(func)));
}

void LEnv::add_builtin_macro(const string &name, lbuiltin func) {
    symbols.insert(std::make_pair(name, LValue::macro(func)));
}

void LEnv::add_builtin_command(const string &name, lbuiltin func) {
    symbols.insert(std::make_pair(name, LValue::command(func)));
}
