#ifndef LENV_HPP
#define LENV_HPP

#include <map>
#include <string>
#include <vector>
#include "builtin.hpp"

struct LValue;

struct LEnv {
    using table_type = std::map<std::string, LValue *>;

    LEnv *parent;
    table_type symbols;

    LEnv();
    LEnv(const LEnv &other);
    explicit LEnv(const LEnv *const other);
    ~LEnv();

    std::vector<std::string> keys() const;
    std::vector<const std::string *> keys(const std::string &prefix) const;

    LValue *get(const std::string &sym) const;
    void put(const std::string &sym, const LValue *const val);
    void def(const std::string &sym, const LValue *const val);

    void add_builtin_function(const std::string &name, lbuiltin func);
    void add_builtin_macro(const std::string &name, lbuiltin func);
    void add_builtin_command(const std::string &name, lbuiltin func);
};

#endif // LENV_HPP
