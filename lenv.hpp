#ifndef LENV_HPP
#define LENV_HPP

#include <string>
#include <unordered_map>
#include "builtin.hpp"

struct lval;

struct lenv {
    using table_type = std::unordered_map<std::string, lval*>;

    lenv *parent;
    table_type symbols;

    lenv();
    lenv(const lenv &other);
    lenv(const lenv *const other);
    ~lenv();

    lval* get(const std::string &sym) const;
    void put(const std::string &sym, const lval *const val);
    void def(const std::string &sym, const lval *const val);
    void add_builtin(const std::string &name, lbuiltin func);
};

#endif // LENV_HPP
