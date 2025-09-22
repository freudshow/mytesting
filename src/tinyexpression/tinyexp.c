#include <stdio.h>
#include <stdlib.h>
#include "tinyexpr.h"

/* An example of calling a C function. */
double my_sum(double a, double b)
{
    printf("Called C function with %f and %f.\n", a, b);
    return a + b;
}

double add_with_context(void *context, double x)
{
    double offset = *(double*) context;
    return x + offset;
}

void testtinyexpr(void)
{
    double offset = 5.0;
    te_variable vars[] = {
                           { "mysum", my_sum, TE_FUNCTION2 },
                           { "addc", add_with_context, TE_CLOSURE1, &offset }
    };

    const char *expression = "cos(60) + mysum(5, 6)";
    printf("Evaluating:\n\t%s\n", expression);

    int err;
    te_expr *n = te_compile(expression, vars, 1, &err);

    if (n)
    {
        te_print(n);
        const double r = te_eval(n);
        printf("Result:\n\t%f\n", r);
        te_free(n);
    }
    else
    {
        /* Show the user where the error is at. */
        printf("\t%*s^\nError near here", err - 1, "");
    }

    const char *expr2 = "3 + 4 * 2 / (1 - 5) ^ 2 ^ 3";
    printf("Evaluating:\n\t%s\n", expr2);
    printf("%f\n", te_interp(expr2, &err));

    int error;
    te_expr *expr = te_compile("addc(3)", vars, 2, &error);
    if (expr)
    {
        double result = te_eval(expr);
        printf("Result: %f\n", result); // Output: Result: 8.000000
        te_free(expr);
    }
    else
    {
        printf("Parse error at %d\n", error);
    }

    const char *expr3 = "3 + -4 * 2 ";
    printf("Evaluating:\n\t%s\n", expr3);
    printf("%f\n", te_interp(expr3, &err));
}
