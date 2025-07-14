#include <stdio.h>
#include <stdlib.h>
#include "tinyexpr.h"

/* An example of calling a C function. */
double my_sum(double a, double b) {
    printf("Called C function with %f and %f.\n", a, b);
    return a + b;
}

void testtinyexpr(void)
{
    te_variable vars[] = {
        {"mysum", my_sum, TE_FUNCTION2}
    };

    const char *expression = "cos(60) + mysum(5, 6)";
    printf("Evaluating:\n\t%s\n", expression);

    int err;
    te_expr *n = te_compile(expression, vars, 1, &err);

    if (n) {
        te_print(n);
        const double r = te_eval(n);
        printf("Result:\n\t%f\n", r);
        te_free(n);
    } else {
        /* Show the user where the error is at. */
        printf("\t%*s^\nError near here", err-1, "");
    }

    const char *expr2 = "3 + 4 * 2 / (1 - 5) ^ 2 ^ 3";
    printf("Evaluating:\n\t%s\n", expr2);
    printf("%f\n", te_interp(expr2, &err));
}
