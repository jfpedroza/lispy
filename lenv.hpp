#ifndef LENV_HPP
#define LENV_HPP

#include <string>
#include <unordered_map>

struct lval;

struct lenv {
    std::unordered_map<std::string, lval*> symbols;

    ~lenv();

    lval* get(const std::string &sym) const;
    void put(const std::string &sym, const lval *const val);
};

#endif // LENV_HPP
