#include <iostream>
#include <cstdlib>
#include "mpc.h"
#include "parsing.h"

using namespace std;

enum class lval_type {
    integer,
    decimal,
    error
};

enum class lval_error {
    div_zero,
    bad_op,
    bad_num,
    int_mod
};

#define LVAL_OPERATOR_BASE(OTHER, E1, E2, E3, E4) \
switch (this->type) { \
    case lval_type::decimal: \
        switch (OTHER.type) { \
            case lval_type::decimal: return E1; \
            case lval_type::integer: return E2; \
            default: return OTHER; \
        } \
    case lval_type::integer: \
        switch (OTHER.type) { \
            case lval_type::decimal: return E3; \
            case lval_type::integer: return E4; \
            default: return OTHER; \
        } \
        default: return *this; \
}

#define LVAL_OPERATOR(OP) \
    LVAL_OPERATOR_BASE(other, \
            lval(this->dec OP other.dec), \
            lval(this->dec OP other.integ), \
            lval(this->integ OP other.dec), \
            lval(this->integ OP other.integ) \
        )

struct lval {
    lval_type type;
    long integ;
    double dec;
    lval_error err;

    lval(long num) {
        this->type = lval_type::integer;
        this->integ = num;
    }

    lval(double num) {
        this->type = lval_type::decimal;
        this->dec = num;
    }

    lval(lval_error err) {
        this->type = lval_type::error;
        this->err = err;
    }

    lval operator+(const lval &other) {
        LVAL_OPERATOR(+)
    }

    lval operator-(const lval &other) {
        LVAL_OPERATOR(-)
    }

    lval operator*(const lval &other) {
        LVAL_OPERATOR(*)
    }

    lval operator/(const lval &other) {
        const lval &x = *this;
        auto y = other;

        LVAL_OPERATOR_BASE(y,
                y.dec == 0 ? lval(lval_error::div_zero) : lval(x.dec / y.dec),
                y.integ == 0 ? lval(lval_error::div_zero) : lval(x.dec / y.integ),
                y.dec == 0 ? lval(lval_error::div_zero) : lval(x.integ / y.dec),
                y.integ == 0 ? lval(lval_error::div_zero) : lval(x.integ / y.integ)
                )
    }

    lval operator%(const lval &other) {
        const lval &x = *this;
        auto y = other;

        LVAL_OPERATOR_BASE(y,
                lval(lval_error::int_mod),
                lval(lval_error::int_mod),
                lval(lval_error::int_mod),
                y.integ == 0 ? lval(lval_error::div_zero) : lval(x.integ % y.integ)
                )
    }


    lval operator^(const lval &other) {
        const lval &x = *this;
        auto y = other;
        LVAL_OPERATOR_BASE(other,
                lval(pow(x.dec, y.dec)),
                lval(pow(x.dec, y.integ)),
                lval(pow(x.integ, y.dec)),
                lval(pow(x.integ, y.integ))
                )
    }

    const lval& min(const lval &y) {
        const lval &x = *this;
        LVAL_OPERATOR_BASE(y,
                x.dec < y.dec ? x : y,
                x.dec < y.integ ? x : y,
                x.integ < y.dec ? x : y,
                x.integ < y.integ ? x : y
            )
    }

    const lval& max(const lval &y) {
        const lval &x = *this;
        LVAL_OPERATOR_BASE(y,
                x.dec > y.dec ? x : y,
                x.dec > y.integ ? x : y,
                x.integ > y.dec ? x : y,
                x.integ > y.integ ? x : y
                )
    }
};

ostream& operator<<(ostream& os, lval value) {
    switch (value.type) {
        case lval_type::integer:
            return os << value.integ;

        case lval_type::decimal:
            return os << value.dec;

        case lval_type::error:
            switch (value.err) {
                case lval_error::div_zero: return os << "Error: Division by zero!";
                case lval_error::bad_op: return os << "Error: Invalid operator!";
                case lval_error::bad_num: return os << "Error: Invalid number!";
                case lval_error::int_mod: return os << "Error: Module operation can only be applied to integers!";
            }
            break;
    }

    return os;
}

/* Use operator string to see which operation to perform */
lval eval_op(lval x, char* op, lval y) {

    if (x.type == lval_type::error) return x;
    if (y.type == lval_type::error) return y;

    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) return x + y;
    if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) return x - y;
    if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) return x * y;
    if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) return x / y;
    if (strcmp(op, "%") == 0 || strcmp(op, "rem") == 0) return x % y;
    if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) return x ^ y;

    if (strcmp(op, "min") == 0) return x.min(y);
    if (strcmp(op, "max") == 0) return x.max(y);

    return lval(lval_error::bad_op);
}

lval eval(mpc_ast_t* t) {

    cout << "Tag: " << t->tag << endl;

    /* If tagged as number return it directly. */
    if (strstr(t->tag, "integer")) {
        /* Check if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval(x) : lval(lval_error::bad_num);
    }

    if (strstr(t->tag, "decimal")) {
        /* Check if there is some error in conversion */
        errno = 0;
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ? lval(x) : lval(lval_error::bad_num);
    }

    /* The operator is always second child. */
    char* op = t->children[1]->contents;

    /* We store the third child in `x` */
    lval x = eval(t->children[2]);

    /* Iterate the remaining children and combining. */
    int i;
    for (i = 3; strstr(t->children[i]->tag, "expr"); i++) {
        x = eval_op(x, op, eval(t->children[i]));
    }

    if (x.type == lval_type::error) return x;

    /* If the operator is - and has a single operand */
    if (x.type == lval_type::integer && strcmp(op, "-") == 0 && i == 3) {
        return lval(-x.integ);
    }

    return x;
}

int main(int argc, char* argv[]) {

    /* Create some parsers */
    mpc_parser_t* Decimal = mpc_new("decimal");
    mpc_parser_t* Integer = mpc_new("integer");
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                   \
        integer   : /-?[0-9]+/ ;                            \
        decimal   : /-?[0-9]+\\.[0-9]+/ ;                   \
        number   : <decimal> | <integer>;                   \
        operator : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"rem\" | \"pow\" | \"min\" | \"max\" ; \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        lispy    : /^/ <operator> <expr>+ /$/ ;             \
        ",
        Decimal, Integer, Number, Operator, Expr, Lispy);

    /* Print Version and Exit Information */
    cout << "Lispy Version 0.0.0.0.4" << endl;
    cout << "Press Ctrl+c to Exit\n" << endl;

    /* In a never ending loop */
    while(true) {
        /* Output our prompt  and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval result = eval((mpc_ast_t*)r.output);
            cout << result << endl;
            mpc_ast_delete((mpc_ast_t*)r.output);
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    /* Undefine and Delete our Parsers */
    mpc_cleanup(6, Decimal, Integer, Number, Operator, Expr, Lispy);
    return 0;
}

