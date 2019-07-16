#ifndef LENV_HPP
#define LENV_HPP

#include <string>
#include <unordered_map>
#include "builtin.hpp"

struct lval;

struct lenv {
    std::unordered_map<std::string, lval*> symbols;

    ~lenv();

    lval* get(const std::string &sym) const;
    void put(const std::string &sym, const lval *const val);
    void add_builtin(const std::string &name, lbuiltin func);
};

#endif // LENV_HPP
