#include "lval.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include "builtin.hpp"
#include "lenv.hpp"
#include "lval_error.hpp"

using std::ostream;
using std::string;
using std::vector;

ostream &operator<<(ostream &os, const lval_type &type) {
    switch (type) {
        case lval_type::integer:
            return os << "Integer";
        case lval_type::decimal:
            return os << "Decimal";
        case lval_type::number:
            return os << "Number";
        case lval_type::boolean:
            return os << "Boolean";
        case lval_type::error:
            return os << "Error";
        case lval_type::symbol:
            return os << "Symbol";
        case lval_type::cname:
            return os << "Command name";
        case lval_type::string:
            return os << "String";
        case lval_type::func:
            return os << "Function";
        case lval_type::macro:
            return os << "Macro";
        case lval_type::command:
            return os << "Command";
        case lval_type::sexpr:
            return os << "S-Expression";
        case lval_type::qexpr:
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

lval::lval(lval_type type) {
    this->type = type;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->dec = 0.0;
    this->integ = 0;
}

lval::lval(long num) {
    this->type = lval_type::integer;
    this->integ = num;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->dec = 0.0;
}

lval::lval(double num) {
    this->type = lval_type::decimal;
    this->dec = num;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->integ = 0;
}

lval::lval(bool boolean) {
    this->type = lval_type::boolean;
    this->boolean = boolean;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->dec = 0.0;
    this->integ = 0;
}

lval::lval(string str) {
    this->type = lval_type::string;
    this->str = str;
    this->body = nullptr;
    this->formals = nullptr;
    this->env = nullptr;
    this->boolean = false;
    this->dec = 0.0;
    this->integ = 0;
}

lval::lval(const lval &other) {
    this->type = other.type;
    switch (this->type) {
        case lval_type::integer:
            this->integ = other.integ;
            break;
        case lval_type::decimal:
            this->dec = other.dec;
            break;
        case lval_type::boolean:
            this->boolean = other.boolean;
            break;
        case lval_type::error:
            this->err = other.err;
            break;
        case lval_type::symbol:
        case lval_type::cname:
            this->sym = other.sym;
            break;
        case lval_type::string:
            this->str = other.str;
            break;
        case lval_type::func:
        case lval_type::macro:
        case lval_type::command:
            if (other.builtin) {
                this->builtin = other.builtin;
            } else {
                this->builtin = nullptr;
                this->env = new lenv(other.env);
                this->formals = new lval(other.formals);
                this->body = new lval(other.body);
            }
            break;
        case lval_type::sexpr:
        case lval_type::qexpr:
            std::transform(other.cells.begin(), other.cells.end(),
                           std::back_inserter(this->cells),
                           [](auto cell) { return new lval(cell); });
            break;
        default:
            break;
    }
}

lval::lval(const lval *const other): lval(*other) {}

lval *lval::symbol(string sym) {
    auto val = new lval(lval_type::symbol);
    val->sym = sym;
    return val;
}

lval *lval::cname(string sym) {
    auto val = new lval(lval_type::cname);
    val->sym = sym;
    return val;
}

lval *lval::error(string err) {
    auto val = new lval(lval_type::error);
    val->err = err;
    return val;
}

lval *lval::function(lbuiltin fun) {
    auto val = new lval(lval_type::func);
    val->builtin = fun;
    return val;
}

lval *lval::function(lval *formals, lval *body) {
    auto val = new lval(lval_type::func);
    val->builtin = nullptr;
    val->formals = formals;
    val->body = body;
    val->env = new lenv();
    return val;
}

lval *lval::macro(lbuiltin fun) {
    auto val = new lval(lval_type::macro);
    val->builtin = fun;
    return val;
}

lval *lval::macro(lval *formals, lval *body) {
    auto val = new lval(lval_type::macro);
    val->builtin = nullptr;
    val->formals = formals;
    val->body = body;
    val->env = new lenv();
    return val;
}

lval *lval::command(lbuiltin fun) {
    auto val = new lval(lval_type::command);
    val->builtin = fun;
    return val;
}

lval *lval::sexpr() {
    auto val = new lval(lval_type::sexpr);
    return val;
}

lval *lval::sexpr(std::initializer_list<lval *> cells) {
    auto val = sexpr();
    val->cells = cells;
    return val;
}

lval *lval::qexpr() {
    auto val = new lval(lval_type::qexpr);
    return val;
}

lval *lval::qexpr(std::initializer_list<lval *> cells) {
    auto val = qexpr();
    val->cells = cells;
    return val;
}

lval::~lval() {
    for (auto cell: cells) {
        delete cell;
    }

    if ((type == lval_type::func || type == lval_type::macro) && !builtin) {
        delete formals;
        delete body;
        delete env;
    }
}

bool lval::is_number() const {
    switch (this->type) {
        case lval_type::integer:
            return true;
        case lval_type::decimal:
            return true;
        default:
            return false;
    }
}

double lval::get_number() const {
    switch (this->type) {
        case lval_type::integer:
            return this->integ;
        case lval_type::decimal:
            return this->dec;
        default:
            return std::numeric_limits<double>::max();
    }
}

lval *lval::pop(const iter &it) {
    auto x = *it;
    cells.erase(it);
    return x;
}

lval *lval::pop(size_t i) {
    auto it = cells.begin();
    for (size_t pos = 0; pos < i; pos++, ++it) {
    }

    return pop(it);
}

lval *lval::pop_first() { return pop(cells.begin()); }

lval *lval::call(lenv *e, lval *a) {
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

            if (this->type == lval_type::macro) {
                for (auto &cell: a->cells) {
                    cell = lval::qexpr({cell});
                }
            }

            env->put(nsym->sym, builtin::list(e, a));
            delete sym;
            delete nsym;
            break;
        }

        auto val = a->pop_first();
        if (this->type == lval_type::macro) {
            val = lval::qexpr({val});
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
        auto val = lval::qexpr();
        env->put(sym->sym, val);
        delete sym;
        delete val;
    }

    if (formals->cells.empty()) {
        env->parent = e;

        auto v = new lval(body);
        return eval_qexpr(env, v);
    } else {
        return new lval(this);
    }
}

lval *lval::take(lval *v, const iter &it) {
    auto x = v->pop(it);
    delete v;
    return x;
}

lval *lval::take(lval *v, size_t i) {
    auto x = v->pop(i);
    delete v;
    return x;
}

lval *lval::take_first(lval *v) { return take(v, v->cells.begin()); }

lval *lval::read_integer(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? new lval(x) : error(lerr::bad_num());
}

lval *lval::read_decimal(mpc_ast_t *t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? new lval(x) : error(lerr::bad_num());
}

lval *lval::read_string(mpc_ast_t *t) {
    t->contents[strlen(t->contents) - 1] = '\0';
    char *unescaped = (char *)malloc(strlen(t->contents + 1) + 1);
    strcpy(unescaped, t->contents + 1);
    unescaped = (char *)mpcf_unescape(unescaped);
    auto str = new lval(string(unescaped));
    free(unescaped);
    return str;
}

lval *lval::read(mpc_ast_t *t) {
    if (strstr(t->tag, "integer")) return read_integer(t);
    if (strstr(t->tag, "decimal")) return read_decimal(t);
    if (strstr(t->tag, "string")) return read_string(t);
    if (strstr(t->tag, "symbol")) return lval::symbol(t->contents);
    if (strstr(t->tag, "cname")) return lval::cname(t->contents);

    lval *x = nullptr;
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

lval *lval::eval(lenv *e, lval *v) {
    if (v->type == lval_type::symbol || v->type == lval_type::cname) {
        auto x = e->get(v->sym);
        delete v;
        return x;
    }

    if (v->type == lval_type::sexpr) return eval_sexpr(e, v);

    return v;
}

lval *lval::eval_sexpr(lenv *e, lval *v) {
    if (v->cells.empty()) return v;

    auto begin = v->cells.begin();
    *begin = eval(e, *begin);

    if (v->cells.size() == 1) {
        auto val = take_first(v);
        if ((*begin)->type == lval_type::command) {
            return val->call(e, lval::sexpr());
        }

        return val;
    }

    auto f = v->pop_first();

    switch (f->type) {
        case lval_type::error:
            delete v;
            return f;

        case lval_type::func: {
            v = eval_cells(e, v);
            if (v->type == lval_type::error) {
                delete f;
                return v;
            }

            auto result = f->call(e, v);
            delete f;

            return result;
        }
        case lval_type::macro:
        case lval_type::command: {
            v->type = lval_type::qexpr;
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

lval *lval::eval_qexpr(lenv *e, lval *v) {
    v->type = lval_type::sexpr;
    return eval_sexpr(e, v);
}

lval *lval::eval_cells(lenv *e, lval *v) {
    std::transform(v->cells.begin(), v->cells.end(), v->cells.begin(),
                   std::bind(eval, e, std::placeholders::_1));

    for (auto it = v->cells.begin(); it != v->cells.end(); ++it) {
        if ((*it)->type == lval_type::error) {
            return take(v, it);
        }
    }

    return v;
}

ostream &lval::print_expr(ostream &os, char open, char close) const {
    os << open;

    for (auto it = cells.begin(); it != cells.end();) {
        os << **it;

        if (++it != cells.end()) os << ' ';
    }

    return os << close;
}

ostream &lval::print_str(ostream &os) const {
    auto s = str.c_str();
    char *escaped = (char *)malloc(str.size() + 1);
    strcpy(escaped, s);
    escaped = (char *)mpcf_escape(escaped);
    os << '\"' << escaped << '\"';
    free(escaped);
    return os;
}

ostream &operator<<(ostream &os, const lval &value) {
    switch (value.type) {
        case lval_type::integer:
            return os << value.integ;

        case lval_type::decimal:
            return os << value.dec;

        case lval_type::boolean:
            return os << (value.boolean ? "true" : "false");

        case lval_type::symbol:
        case lval_type::cname:
            return os << value.sym;

        case lval_type::string:
            return value.print_str(os);

        case lval_type::func:
            if (value.builtin) {
                return os << "<builtin function>";
            } else {
                return os << "(\\ " << *value.formals << ' ' << *value.body
                          << ')';
            }
            break;
        case lval_type::macro:
            if (value.builtin) {
                return os << "<builtin macro>";
            } else {
                return os << "(\\! " << *value.formals << ' ' << *value.body
                          << ')';
            }
            break;
        case lval_type::command:
            return os << "<command>";
        case lval_type::error:
            return os << "Error: " << value.err;

        case lval_type::sexpr:
            return value.print_expr(os, '(', ')');

        case lval_type::qexpr:
            return value.print_expr(os, '{', '}');

        default:
            return os;
    }
}

bool lval::operator==(const lval &other) const {
    if (this->is_number() && other.is_number()) {
        switch (this->type) {
            case lval_type::decimal:
                switch (other.type) {
                    case lval_type::decimal:
                        return this->dec == other.dec;
                    case lval_type::integer:
                        return this->dec == other.integ;
                    default:
                        return false;
                }
            case lval_type::integer:
                switch (other.type) {
                    case lval_type::decimal:
                        return this->integ == other.dec;
                    case lval_type::integer:
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
        case lval_type::boolean:
            return this->boolean == other.boolean;
        case lval_type::error:
            return this->err == other.err;
        case lval_type::symbol:
        case lval_type::cname:
            return this->sym == other.sym;
        case lval_type::string:
            return this->str == other.str;

        case lval_type::func:
        case lval_type::macro:
        case lval_type::command:
            if (this->builtin && other.builtin) {
                auto a = this->builtin.target<lval *(*)(lenv *, lval *)>();
                auto b = other.builtin.target<lval *(*)(lenv *, lval *)>();
                return *a == *b;
            } else if (!this->builtin && !other.builtin) {
                return *this->formals == *other.formals &&
                       *this->body == *other.body;
            } else {
                return false;
            }

        case lval_type::sexpr:
        case lval_type::qexpr:
            if (this->cells.size() != other.cells.size()) return false;

            return std::equal(this->cells.begin(), this->cells.end(),
                              other.cells.begin(),
                              [](auto a, auto b) { return *a == *b; });

        default:
            return false;
    }
}

bool lval::operator!=(const lval &other) const { return !(*this == other); }
