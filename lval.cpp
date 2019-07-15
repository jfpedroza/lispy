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

lval::lval(string sym) {
    this->type = lval_type::symbol;
    this->sym = sym;
}

lval::lval(lbuiltin fun) {
    this->type = lval_type::func;
    this->fun = fun;
}

lval::lval(const lval &other) {
    this->type = other.type;
    switch (this->type) {
        case lval_type::integer: this->integ = other.integ; break;
        case lval_type::decimal: this->dec = other.dec; break;
        case lval_type::error: this->err = other.err; break;
        case lval_type::symbol: this->sym = other.sym; break;
        case lval_type::func: this->fun = other.fun; break;
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
    if (strstr(t->tag, "symbol")) return new lval(t->contents);

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
        delete f;
        delete v;
        return error(lerr::sexpr_not_function());
    }

    auto result = f->fun(e, v);
    delete f;

    return result;
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

        case lval_type::symbol:
            return os << value.sym;

        case lval_type::func:
            return os << "<function>";

        case lval_type::error:
            return os << "Error: " << value.err;

        case lval_type::sexpr:
            return value.print_expr(os, '(', ')');

        case lval_type::qexpr:
            return value.print_expr(os, '{', '}');
    }

    return os;
}
