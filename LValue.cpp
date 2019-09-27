#include "LValue.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include "LEnv.hpp"
#include "builtin.hpp"
#include "lval_error.hpp"

using std::ostream;
using std::string;
using std::vector;

ostream &operator<<(ostream &os, const LValue::Type &type) {
    switch (type) {
        case LValue::Type::integer:
            return os << "Integer";
        case LValue::Type::decimal:
            return os << "Decimal";
        case LValue::Type::number:
            return os << "Number";
        case LValue::Type::boolean:
            return os << "Boolean";
        case LValue::Type::error:
            return os << "Error";
        case LValue::Type::symbol:
            return os << "Symbol";
        case LValue::Type::cname:
            return os << "Command name";
        case LValue::Type::string:
            return os << "String";
        case LValue::Type::func:
            return os << "Function";
        case LValue::Type::macro:
            return os << "Macro";
        case LValue::Type::command:
            return os << "Command";
        case LValue::Type::sexpr:
            return os << "S-Expression";
        case LValue::Type::qexpr:
            return os << "Q-Expression";
        default:
            return os << "Unknown";
    }
}

vector<string> content_skip_strings = {"(", ")", "{", "}"};
bool should_skip_child(mpc_ast_t *child) {
    auto end = content_skip_strings.end();
    auto it = std::find(content_skip_strings.begin(), end, child->contents);
    return it != end;
}

LValue::LValue(Type type) {
    this->type = type;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->dec = 0.0;
    this->integ = 0;
}

LValue::LValue(long num) {
    this->type = Type::integer;
    this->integ = num;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->dec = 0.0;
}

LValue::LValue(double num) {
    this->type = Type::decimal;
    this->dec = num;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->integ = 0;
}

LValue::LValue(bool boolean) {
    this->type = Type::boolean;
    this->boolean = boolean;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->dec = 0.0;
    this->integ = 0;
}

LValue::LValue(string str) {
    this->type = Type::string;
    this->str = str;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->dec = 0.0;
    this->integ = 0;
}

LValue::LValue(const LValue &other) {
    this->type = other.type;
    switch (this->type) {
        case Type::integer:
            this->integ = other.integ;
            break;
        case Type::decimal:
            this->dec = other.dec;
            break;
        case Type::boolean:
            this->boolean = other.boolean;
            break;
        case Type::error:
            this->err = other.err;
            break;
        case Type::symbol:
        case Type::cname:
            this->sym = other.sym;
            break;
        case Type::string:
            this->str = other.str;
            break;
        case Type::func:
        case Type::macro:
        case Type::command:
            if (other.builtin) {
                this->builtin = other.builtin;
            } else {
                this->builtin = nullptr;
                this->env = new LEnv(other.env);
                this->formals = new LValue(other.formals);
                this->body = new LValue(other.body);
            }
            break;
        case Type::sexpr:
        case Type::qexpr:
            std::transform(other.cells.begin(), other.cells.end(),
                           std::back_inserter(this->cells),
                           [](auto cell) { return new LValue(cell); });
            break;
        default:
            break;
    }
}

LValue::LValue(const LValue *const other): LValue(*other) {}

LValue *LValue::symbol(string sym) {
    auto val = new LValue(Type::symbol);
    val->sym = sym;
    return val;
}

LValue *LValue::cname(string sym) {
    auto val = new LValue(Type::cname);
    val->sym = sym;
    return val;
}

LValue *LValue::error(string err) {
    auto val = new LValue(Type::error);
    val->err = err;
    return val;
}

LValue *LValue::function(lbuiltin fun) {
    auto val = new LValue(Type::func);
    val->builtin = fun;
    return val;
}

LValue *LValue::function(LValue *formals, LValue *body) {
    auto val = new LValue(Type::func);
    val->builtin = nullptr;
    val->formals = formals;
    val->body = body;
    val->env = new LEnv();
    return val;
}

LValue *LValue::macro(lbuiltin fun) {
    auto val = new LValue(Type::macro);
    val->builtin = fun;
    return val;
}

LValue *LValue::macro(LValue *formals, LValue *body) {
    auto val = new LValue(Type::macro);
    val->builtin = nullptr;
    val->formals = formals;
    val->body = body;
    val->env = new LEnv();
    return val;
}

LValue *LValue::command(lbuiltin fun) {
    auto val = new LValue(Type::command);
    val->builtin = fun;
    return val;
}

LValue *LValue::sexpr() {
    auto val = new LValue(Type::sexpr);
    return val;
}

LValue *LValue::sexpr(std::initializer_list<LValue *> cells) {
    auto val = sexpr();
    val->cells = cells;
    return val;
}

LValue *LValue::qexpr() {
    auto val = new LValue(Type::qexpr);
    return val;
}

LValue *LValue::qexpr(std::initializer_list<LValue *> cells) {
    auto val = qexpr();
    val->cells = cells;
    return val;
}

LValue::~LValue() {
    for (auto cell: cells) {
        delete cell;
    }

    if ((type == Type::func || type == Type::macro) && !builtin) {
        delete formals;
        delete body;
        delete env;
    }
}

bool LValue::is_number() const {
    switch (this->type) {
        case Type::integer:
            return true;
        case Type::decimal:
            return true;
        default:
            return false;
    }
}

double LValue::get_number() const {
    switch (this->type) {
        case Type::integer:
            return this->integ;
        case Type::decimal:
            return this->dec;
        default:
            return std::numeric_limits<double>::max();
    }
}

LValue *LValue::pop(const iter &it) {
    auto x = *it;
    cells.erase(it);
    return x;
}

LValue *LValue::pop(size_t i) {
    auto it = cells.begin();
    for (size_t pos = 0; pos < i; pos++, ++it) {
    }

    return pop(it);
}

LValue *LValue::pop_first() { return pop(cells.begin()); }

LValue *LValue::call(LEnv *e, LValue *a) {
    if (builtin) return builtin(e, a);

    auto given = a->cells.size();
    auto total = formals->cells.size();

    while (!a->cells.empty()) {
        if (formals->cells.empty()) {
            delete a;
            return error(lerr::too_many_args(given, total));
        }

        auto sym = formals->pop_first();

        if (sym->sym == "&") {
            if (formals->cells.size() != 1) {
                delete a;
                return error(lerr::function_format_invalid());
            }

            auto nsym = formals->pop_first();

            if (this->type == Type::macro) {
                for (auto &cell: a->cells) {
                    cell = LValue::qexpr({cell});
                }
            }

            env->put(nsym->sym, builtin::list(e, a));
            delete sym;
            delete nsym;
            break;
        }

        auto val = a->pop_first();
        if (this->type == Type::macro) {
            val = LValue::qexpr({val});
        }

        env->put(sym->sym, val);
        delete sym;
        delete val;
    }

    delete a;

    if (!formals->cells.empty() && formals->cells.front()->sym == "&") {
        if (formals->cells.size() != 2) {
            return error(lerr::function_format_invalid());
        }

        delete formals->pop_first();
        auto sym = formals->pop_first();
        auto val = LValue::qexpr();
        env->put(sym->sym, val);
        delete sym;
        delete val;
    }

    if (formals->cells.empty()) {
        env->parent = e;

        auto v = new LValue(body);
        return eval_qexpr(env, v);
    } else {
        return new LValue(this);
    }
}

LValue *LValue::take(LValue *v, const iter &it) {
    auto x = v->pop(it);
    delete v;
    return x;
}

LValue *LValue::take(LValue *v, size_t i) {
    auto x = v->pop(i);
    delete v;
    return x;
}

LValue *LValue::take_first(LValue *v) { return take(v, v->cells.begin()); }

LValue *LValue::read_integer(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? new LValue(x) : error(lerr::bad_num());
}

LValue *LValue::read_decimal(mpc_ast_t *t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? new LValue(x) : error(lerr::bad_num());
}

LValue *LValue::read_string(mpc_ast_t *t) {
    t->contents[strlen(t->contents) - 1] = '\0';
    char *unescaped = (char *)malloc(strlen(t->contents + 1) + 1);
    strcpy(unescaped, t->contents + 1);
    unescaped = (char *)mpcf_unescape(unescaped);
    auto str = new LValue(string(unescaped));
    free(unescaped);
    return str;
}

LValue *LValue::read(mpc_ast_t *t) {
    if (strstr(t->tag, "integer")) return read_integer(t);
    if (strstr(t->tag, "decimal")) return read_decimal(t);
    if (strstr(t->tag, "string")) return read_string(t);
    if (strstr(t->tag, "symbol")) return LValue::symbol(t->contents);
    if (strstr(t->tag, "cname")) return LValue::cname(t->contents);

    LValue *x = nullptr;
    if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
        x = sexpr();
    } else if (strstr(t->tag, "qexpr")) {
        x = qexpr();
    }

    for (int i = 0; i < t->children_num; i++) {
        if (should_skip_child(t->children[i])) continue;
        if (strcmp(t->children[i]->tag, "regex") == 0) continue;
        if (strstr(t->children[i]->tag, "comment")) continue;

        x->cells.push_back(read(t->children[i]));
    }

    return x;
}

LValue *LValue::eval(LEnv *e, LValue *v) {
    if (v->type == Type::symbol || v->type == Type::cname) {
        auto x = e->get(v->sym);
        delete v;
        return x;
    }

    if (v->type == Type::sexpr) return eval_sexpr(e, v);

    return v;
}

LValue *LValue::eval_sexpr(LEnv *e, LValue *v) {
    if (v->cells.empty()) return v;

    auto begin = v->cells.begin();
    *begin = eval(e, *begin);

    if (v->cells.size() == 1) {
        auto val = take_first(v);
        if ((*begin)->type == Type::command) {
            return val->call(e, LValue::sexpr());
        }

        return val;
    }

    auto f = v->pop_first();

    switch (f->type) {
        case Type::error:
            delete v;
            return f;

        case Type::func: {
            v = eval_cells(e, v);
            if (v->type == Type::error) {
                delete f;
                return v;
            }

            auto result = f->call(e, v);
            delete f;

            return result;
        }
        case Type::macro:
        case Type::command: {
            v->type = Type::qexpr;
            auto result = f->call(e, v);
            delete f;

            return result;
        }
        default:
            auto type = f->type;
            delete f;
            delete v;
            return error(lerr::sexpr_not_function(type));
    }
}

LValue *LValue::eval_qexpr(LEnv *e, LValue *v) {
    v->type = Type::sexpr;
    return eval_sexpr(e, v);
}

LValue *LValue::eval_cells(LEnv *e, LValue *v) {
    std::transform(v->cells.begin(), v->cells.end(), v->cells.begin(),
                   std::bind(eval, e, std::placeholders::_1));

    for (auto it = v->cells.begin(); it != v->cells.end(); ++it) {
        if ((*it)->type == Type::error) {
            return take(v, it);
        }
    }

    return v;
}

ostream &LValue::print_expr(ostream &os, char open, char close) const {
    os << open;

    for (auto it = cells.begin(); it != cells.end();) {
        os << **it;

        if (++it != cells.end()) os << ' ';
    }

    return os << close;
}

ostream &LValue::print_str(ostream &os) const {
    auto s = str.c_str();
    char *escaped = (char *)malloc(str.size() + 1);
    strcpy(escaped, s);
    escaped = (char *)mpcf_escape(escaped);
    os << '\"' << escaped << '\"';
    free(escaped);
    return os;
}

ostream &operator<<(ostream &os, const LValue &value) {
    switch (value.type) {
        case LValue::Type::integer:
            return os << value.integ;

        case LValue::Type::decimal:
            return os << value.dec;

        case LValue::Type::boolean:
            return os << (value.boolean ? "true" : "false");

        case LValue::Type::symbol:
        case LValue::Type::cname:
            return os << value.sym;

        case LValue::Type::string:
            return value.print_str(os);

        case LValue::Type::func:
            if (value.builtin) {
                return os << "<builtin function>";
            } else {
                return os << "(\\ " << *value.formals << ' ' << *value.body
                          << ')';
            }
            break;
        case LValue::Type::macro:
            if (value.builtin) {
                return os << "<builtin macro>";
            } else {
                return os << "(\\! " << *value.formals << ' ' << *value.body
                          << ')';
            }
            break;
        case LValue::Type::command:
            return os << "<command>";
        case LValue::Type::error:
            return os << "Error: " << value.err;

        case LValue::Type::sexpr:
            return value.print_expr(os, '(', ')');

        case LValue::Type::qexpr:
            return value.print_expr(os, '{', '}');

        default:
            return os;
    }
}

bool LValue::operator==(const LValue &other) const {
    if (this->is_number() && other.is_number()) {
        switch (this->type) {
            case Type::decimal:
                switch (other.type) {
                    case Type::decimal:
                        return this->dec == other.dec;
                    case Type::integer:
                        return this->dec == other.integ;
                    default:
                        return false;
                }
            case Type::integer:
                switch (other.type) {
                    case Type::decimal:
                        return this->integ == other.dec;
                    case Type::integer:
                        return this->integ == other.integ;
                    default:
                        return false;
                }
            default:
                return false;
        }
    }

    if (this->type != other.type) return false;

    switch (this->type) {
        case Type::boolean:
            return this->boolean == other.boolean;
        case Type::error:
            return this->err == other.err;
        case Type::symbol:
        case Type::cname:
            return this->sym == other.sym;
        case Type::string:
            return this->str == other.str;

        case Type::func:
        case Type::macro:
        case Type::command:
            if (this->builtin && other.builtin) {
                auto a = this->builtin.target<LValue *(*)(LEnv *, LValue *)>();
                auto b = other.builtin.target<LValue *(*)(LEnv *, LValue *)>();
                return *a == *b;
            } else if (!this->builtin && !other.builtin) {
                return *this->formals == *other.formals &&
                       *this->body == *other.body;
            } else {
                return false;
            }

        case Type::sexpr:
        case Type::qexpr:
            if (this->cells.size() != other.cells.size()) return false;

            return std::equal(this->cells.begin(), this->cells.end(),
                              other.cells.begin(),
                              [](auto a, auto b) { return *a == *b; });

        default:
            return false;
    }
}

bool LValue::operator!=(const LValue &other) const { return !(*this == other); }
