#include <algorithm>
#include <string>
#include <vector>
#include "lval.hpp"
#include "lval_error.hpp"
#include "lenv.hpp"
#include "builtin.hpp"

using std::string;
using std::vector;
using std::ostream;

ostream& operator<<(ostream &os, const lval_type &type) {
    switch (type) {
        case lval_type::integer: return os << "Integer";
        case lval_type::decimal: return os << "Decimal";
        case lval_type::number: return os << "Number";
        case lval_type::boolean: return os << "Boolean";
        case lval_type::error: return os << "Error";
        case lval_type::symbol: return os << "Symbol";
        case lval_type::func: return os << "Function";
        case lval_type::sexpr: return os << "S-Expression";
        case lval_type::qexpr: return os << "Q-Expression";
        default: return os << "Unknown";
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
}

lval::lval(long num) {
    this->type = lval_type::integer;
    this->integ = num;
}

lval::lval(double num) {
    this->type = lval_type::decimal;
    this->dec = num;
}

lval::lval(bool boolean) {
    this->type = lval_type::boolean;
    this->boolean = boolean;
}

lval::lval(string sym) {
    this->type = lval_type::symbol;
    this->sym = sym;
}

lval::lval(lbuiltin fun) {
    this->type = lval_type::func;
    this->builtin = fun;
}

lval::lval(lval *formals, lval *body) {
    this->type = lval_type::func;
    this->builtin = nullptr;
    this->formals = formals;
    this->body = body;
    this->env = new lenv();
}

lval::lval(const lval &other) {
    this->type = other.type;
    switch (this->type) {
        case lval_type::integer: this->integ = other.integ; break;
        case lval_type::decimal: this->dec = other.dec; break;
        case lval_type::boolean: this->boolean = other.boolean; break;
        case lval_type::error: this->err = other.err; break;
        case lval_type::symbol: this->sym = other.sym; break;
        case lval_type::func:
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
            this->cells = cell_type(other.cells.size(), nullptr);
            std::transform(other.cells.begin(), other.cells.end(), this->cells.begin(), [](auto cell) { return new lval(cell); });
            break;
    }
}

lval::lval(const lval *const other): lval(*other) {}

lval* lval::error(string err) {
    auto val = new lval(lval_type::error);
    val->err = err;
    return val;
}

lval* lval::sexpr() {
    auto val = new lval(lval_type::sexpr);
    return val;
}

lval* lval::qexpr() {
    auto val = new lval(lval_type::qexpr);
    return val;
}

lval::~lval() {
    for(auto cell: cells) {
        delete cell;
    }

    if (type == lval_type::func && !builtin) {
        delete formals;
        delete body;
        delete env;
    }
}

bool lval::is_number() const {
    switch (this->type) {
        case lval_type::integer: return true;
        case lval_type::decimal: return true;
        default: return false;
    }
}

double lval::get_number() const {
    switch (this->type) {
        case lval_type::integer: return this->integ;
        case lval_type::decimal: return this->dec;
        default:
            return std::numeric_limits<double>::max();
    }
}

lval* lval::pop(const iter &it) {
    auto x = *it;
    cells.erase(it);
    return x;
}

lval* lval::pop(size_t i) {
    auto it = cells.begin();
    for (size_t pos = 0; pos < i; pos++, ++it) {}

    return pop(it);
}

lval* lval::pop_first() {
    return pop(cells.begin());
}

lval* lval::call(lenv *e, lval *a) {
    if (builtin) return builtin(e, a);

    auto given = a->cells.size();
    auto total = formals->cells.size();

    while(!a->cells.empty()) {
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
            env->put(nsym->sym, builtin::list(e, a));
            delete sym;
            delete nsym;
            break;
        }

        auto val = a->pop_first();
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

lval* lval::take(lval *v, const iter &it) {
    auto x = v->pop(it);
    delete v;
    return x;
}

lval* lval::take(lval *v, size_t i) {
    auto x = v->pop(i);
    delete v;
    return x;
}

lval* lval::take_first(lval *v) {
    return take(v, v->cells.begin());
}

lval* lval::read_integer(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ?
        new lval(x) : error(lerr::bad_num());
}

lval* lval::read_decimal(mpc_ast_t *t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ?
        new lval(x) : error(lerr::bad_num());
}

lval* lval::read(mpc_ast_t *t) {
    if (strstr(t->tag, "integer")) return read_integer(t);
    if (strstr(t->tag, "decimal")) return read_decimal(t);
    if (strstr(t->tag, "symbol")) return new lval(string(t->contents));

    lval *x = nullptr;
    if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
        x = sexpr();
    } else if (strstr(t->tag, "qexpr")) {
        x = qexpr();
    }

    for (int i = 0; i < t->children_num; i++) {
        if (should_skip_child(t->children[i])) continue;
        if (strcmp(t->children[i]->tag,  "regex") == 0) continue;

        x->cells.push_back(read(t->children[i]));
    }

    return x;
}

lval* lval::eval(lenv *e, lval *v) {
    if (v->type == lval_type::symbol) {
        auto x = e->get(v->sym);
        delete v;
        return x;
    }

    if (v->type == lval_type::sexpr) return eval_sexpr(e, v);

    return v;
}

lval* lval::eval_sexpr(lenv *e, lval *v) {
    std::transform(v->cells.begin(), v->cells.end(), v->cells.begin(), std::bind(eval, e, std::placeholders::_1));

    for (auto it = v->cells.begin(); it != v->cells.end(); ++it) {
        if ((*it)->type == lval_type::error) return take(v, it);
    }

    if (v->cells.empty()) return v;

    if (v->cells.size() == 1) return take_first(v);

    auto f = v->pop_first();
    if (f->type != lval_type::func) {
        auto type = f->type;
        delete f;
        delete v;
        return error(lerr::sexpr_not_function(type));
    }

    auto result = f->call(e, v);
    delete f;

    return result;
}

lval* lval::eval_qexpr(lenv *e, lval *v) {
    v->type = lval_type::sexpr;
    return eval_sexpr(e, v);
}

ostream& lval::print_expr(ostream &os, char open, char close) const {
    os << open;

    for (auto it = cells.begin(); it != cells.end();) {
        os << **it;

        if (++it != cells.end()) os << ' ';
    }

    return os << close;
}

ostream& operator<<(ostream &os, const lval &value) {
    switch (value.type) {
        case lval_type::integer:
            return os << value.integ;

        case lval_type::decimal:
            return os << value.dec;

        case lval_type::boolean:
            return os << (value.boolean ? "true" : "false");

        case lval_type::symbol:
            return os << value.sym;

        case lval_type::func:
            if (value.builtin) {
                return os << "<builtin>";
            } else {
                os << "(\\ " << *value.formals << ' ' << *value.body << ')';
            }
            break;
        case lval_type::error:
            return os << "Error: " << value.err;

        case lval_type::sexpr:
            return value.print_expr(os, '(', ')');

        case lval_type::qexpr:
            return value.print_expr(os, '{', '}');
    }

    return os;
}

bool lval::operator==(const lval &other) const {
    if (this->is_number() && other.is_number()) {
        switch (this->type) {
            case lval_type::decimal:
                switch (other.type) {
                    case lval_type::decimal: return this->dec == other.dec;
                    case lval_type::integer: return this->dec == other.integ;
                    default: return false;
                }
            case lval_type::integer:
                switch (other.type) {
                    case lval_type::decimal: return this->integ == other.dec;
                    case lval_type::integer: return this->integ == other.integ;
                    default: return false;
                }
            default: return false;
        }
    }

    if (this->type != other.type) return false;

    switch (this->type) {
        case lval_type::boolean: return this->boolean == other.boolean;
        case lval_type::error: return this->err == other.err;
        case lval_type::symbol: return this->sym == other.sym;

        case lval_type::func:
            if (this->builtin && other.builtin) {
                auto a = this->builtin.target<lval*(*)(lenv*, lval*)>();
                auto b = other.builtin.target<lval*(*)(lenv*, lval*)>();
                return *a == *b;
            } else if (!this->builtin && !other.builtin) {
                return *this->formals == *other.formals && *this->body == *other.body;
            } else {
                return false;
            }

        case lval_type::sexpr:
        case lval_type::qexpr:
            if (this->cells.size() != other.cells.size()) return false;

            return std::equal(
                this->cells.begin(),
                this->cells.end(),
                other.cells.begin(),
                [](auto a, auto b) {return *a == *b;});

        default: return false;
    }
}

bool lval::operator!=(const lval &other) const {
    return !(*this == other);
}
