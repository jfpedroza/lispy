#include <iostream>
#include <cstdlib>
#include "mpc.h"
#include "parsing.h"

using namespace std;

enum class lval_type {
    number,
    error
};

enum class lval_error {
    div_zero,
    bad_op,
    bad_num
};

struct lval {
    lval_type type;
    long num;
    lval_error err;

    lval(long num) {
        this->type = lval_type::number;
        this->num = num;
    }

    lval(lval_error err) {
        this->type = lval_type::error;
        this->err = err;
    }
};

ostream& operator<<(ostream& os, lval value) {
    switch (value.type) {
        case lval_type::number:
            return os << value.num;

        case lval_type::error:
            switch (value.err) {
                case lval_error::div_zero: return os << "Error: Division by zero!";
                case lval_error::bad_op: return os << "Error: Invalid operator!";
                case lval_error::bad_num: return os << "Error: Invalid number!";
            }
            break;
    }

    return os;
}

/* Use operator string to see which operation to perform */
lval eval_op(lval x, char* op, lval y) {

    if (x.type == lval_type::error) return x;
    if (y.type == lval_type::error) return y;

    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) return lval(x.num + y.num);
    if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) return lval(x.num - y.num);
    if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) return lval(x.num * y.num);
    if (strcmp(op, "%") == 0 || strcmp(op, "rem") == 0) return lval(x.num % y.num);
    if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) return lval(pow(x.num, y.num));

    if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
        return y.num == 0 ? lval(lval_error::div_zero) : lval(x.num / y.num);
    }

    if (strcmp(op, "min") == 0) return x.num < y.num ? x : y;
    if (strcmp(op, "max") == 0) return x.num > y.num ? x : y;

    return lval(lval_error::bad_op);
}

lval eval(mpc_ast_t* t) {

    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number")) {
        /* Check if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
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

    /* If the operator is - and has a single operand */
    if (x.type == lval_type::number && strcmp(op, "-") == 0 && i == 3) {
        return lval(-x.num);
    }

    return x;
}

int main(int argc, char* argv[]) {
    
    /* Create some parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                   \
        number   : /-?[0-9]+(\\.[0-9]+)?/ ;                             \
        operator : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"rem\" | \"pow\" | \"min\" | \"max\" ; \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        lispy    : /^/ <operator> <expr>+ /$/ ;             \
        ",
        Number, Operator, Expr, Lispy);

    /* Print Version and Exit Information */
    cout << "Lispy Version 0.0.0.0.1" << endl;
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
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}

