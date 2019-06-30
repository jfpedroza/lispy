#include <iostream>
#include <cstdlib>
#include "mpc.h"

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

using namespace std;

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return pow(x, y); }
    if (strcmp(op, "add") == 0) { return x + y; }
    if (strcmp(op, "sub") == 0) { return x - y; }
    if (strcmp(op, "mul") == 0) { return x * y; }
    if (strcmp(op, "div") == 0) { return x / y; }
    if (strcmp(op, "rem") == 0) { return x % y; }
    if (strcmp(op, "pow") == 0) { return pow(x, y); }

    if (strcmp(op, "min") == 0) {
        if (x < y) {
            return x;
        } else {
            return y;
        }
    }

    if (strcmp(op, "max") == 0) {
        if (x > y) {
            return x;
        } else {
            return y;
        }
    }

    return 0;
}

long eval(mpc_ast_t* t) {

    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* The operator is always second child. */
    char* op = t->children[1]->contents;

    /* We store the third child in `x` */
    long x = eval(t->children[2]);

    /* Iterate the remaining children and combining. */
    for (int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
        x = eval_op(x, op, eval(t->children[i]));
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
    puts("Lispy Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    /* In a never ending loop */
    while(true) {
        /* Output our prompt  and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            long result = eval((mpc_ast_t*)r.output);
            printf("%li\n", result);
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

