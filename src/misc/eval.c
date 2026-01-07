#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

#define EVAL_USE_SHORT_CIRCUIT      0       // enable short-circuit evaluation

typedef enum {
    T_NUM,          // integer number or real number
    T_REALDB,       // real database reference #id
    T_IDENT,        // identifier (function name or variable)
    T_PLUS,         // '+'
    T_MINUS,        // '-'
    T_MUL,          // '*'
    T_DIV,          // '/'
    T_LP,           // '('
    T_RP,           // ')'
    T_NOT,          // '!'
    T_NEQ,          // '!='
    T_ANDAND,       // '&&'
    T_OROR,         // '||'
    T_GT,           // '>'
    T_GTE,          // '>='
    T_LT,           // '<'
    T_LTE,          // '<='
    T_EQ,           // '=='
    T_AMP,          // '&'
    T_PIPE,         // '|'
    T_CARET,        // '^'
    T_TILDE,        // '~'
    T_LSHIFT,       // '<<'
    T_RSHIFT,       // '>>'
    T_ASSIGN,       // '='
    T_COMMA,        // ','
    T_EOF,          // end of input
    T_INVALID       // invalid token
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    double num;
    int pos;
} Token;

typedef struct {
    Token *arr;
    int sz;
    int cap;
    int idx;
} TokenList;

static void tlist_init(TokenList *t)
{
    t->sz = 0;
    t->cap = 16;
    t->arr = malloc(sizeof(Token) * t->cap);
    t->idx = 0;
}

static void tlist_push(TokenList *t, Token tk)
{
    if (t->sz == t->cap)
    {
        t->cap *= 2;
        t->arr = realloc(t->arr, sizeof(Token) * t->cap);
    }

    t->arr[t->sz++] = tk;
}

/***********************************
 * 函数名: tlist_peek
 * 功能: 查看下一个Token但不移动索引
 * -----------------------------------
 * 输入参数: t - Token列表
 * 输出参数: 无
 * -----------------------------------
 * @return - 下一个Token
 */
static Token tlist_peek(TokenList *t)
{
    if (t->idx < t->sz)
    {
        return t->arr[t->idx];
    }

    Token eof = { T_EOF, NULL, 0, 0 };
    return eof;
}

/***************************************
 * 函数名: tlist_next
 * 功能: 获取下一个Token并移动索引
 * -------------------------------------
 * 输入参数: t - Token列表
 * 输出参数: 无
 * -------------------------------------
 * @return - 下一个Token
 **************************************/
static Token tlist_next(TokenList *t)
{
    if (t->idx < t->sz)
    {
        return t->arr[t->idx++];
    }

    Token eof = { T_EOF, NULL, 0, 0 };
    return eof;
}

/***************************************
 * 函数名: tlist_free
 * 功能: 释放Token列表
 * -------------------------------------
 * 输入参数: t - Token列表
 * 输出参数: 无
 * -------------------------------------
 * return: 无
 ***************************************/
static void tlist_free(TokenList *t)
{
    for (int i = 0; i < t->sz; i++)
    {
        if (t->arr[i].text)
        {
            free(t->arr[i].text);
        }
    }

    free(t->arr);
}

// runtime mapping
typedef struct {
    int id;
    double val;
} RtEntry;

typedef struct {
    RtEntry *arr;
    int sz;
    int cap;
} RtMap;

static RtMap s_rt = { 0 };

static void rt_init(int cap)
{
    RtMap *m = &s_rt;
    m->sz = 0;
    m->cap = cap;
    m->arr = malloc(sizeof(RtEntry) * m->cap);
}

static void rt_set(int id, double v)
{
    RtMap *m = &s_rt;

    for (int i = 0; i < m->sz; i++)
    {
        if (m->arr[i].id == id)
        {
            m->arr[i].val = v;
            return;
        }
    }

    if (m->sz == m->cap)
    {
        m->cap *= 2;
        m->arr = realloc(m->arr, sizeof(RtEntry) * m->cap);
    }

    m->arr[m->sz].id = id;
    m->arr[m->sz].val = v;
    m->sz++;
}

static double rt_get(int id)
{
    RtMap *m = &s_rt;

    for (int i = 0; i < m->sz; i++)
    {
        if (m->arr[i].id == id)
        {
            printf("rt_get: id=%d, val=%g\n", id, m->arr[i].val);
            return m->arr[i].val;
        }
    }

    return 0.0;
}

static void rt_free()
{
    RtMap *m = &s_rt;

    free(m->arr);
}

/******************
 * AST node types
 ******************/
typedef enum {
    N_NUMBER,           // numeric literal
    N_REAL_DATABASE,    // real database reference
    N_UNARY,            // unary operation
    N_BINARY,           // binary operation
    N_FUNC,             // function call
    N_ASSIGN            // assignment
} NodeType;

/******************
 * Unary operators
 ******************/
typedef enum {
    U_NEG,              // unary minus
    U_NOT,              // logical NOT
    U_BITNOT            // bitwise NOT
} UnaryOp;

/******************
 * Binary operators
 ******************/
typedef enum {
    B_ADD,
    B_SUB,
    B_MUL,
    B_DIV,
    B_LSHIFT,
    B_RSHIFT,
    B_GT,
    B_GTE,
    B_LT,
    B_LTE,
    B_EQ,
    B_NEQ,
    B_BITAND,
    B_BITXOR,
    B_BITOR,
    B_ANDAND,
    B_OROR
} BinaryOp;

/********************************************
 * AST(Abstract Syntax Tree) Node definition
 ********************************************/
typedef struct ASTNodeStruct {
    NodeType type;                  // node type
    int pos;                        // position in input for errors
    union {
        double number;              // for N_NUMBER
        int realDataBaseId;         // for N_REAL_DATABASE
        struct {
            UnaryOp op;             // operator
            struct ASTNodeStruct *child;     // operand
        } unary;                    // for N_UNARY
        struct {
            BinaryOp op;            // operator
            struct ASTNodeStruct *left;      // left operand
            struct ASTNodeStruct *right;     // right operand
        } binary;                   // for N_BINARY
        struct {
            char *name;             // function name
            void *funcPtr;          // function pointer
            struct ASTNodeStruct **args;     // argument nodes
            int argc;               // number of arguments
        } func;                     // for N_FUNC
        struct {
            int id;                 // real database id
            struct ASTNodeStruct *rhs;       // right-hand side expression
        } assign;                   // for N_ASSIGN
    } v;                            // value
} ASTNode_s;

/**********************************
 * 函数名: pi
 * 功能: 返回圆周率值
 * -------------------------------
 * 输入参数: 无
 * 输出参数: 无
 * -------------------------------
 * @return: 圆周率值
 **********************************/
static double pi(void)
{
    return 3.14159265358979323846;
}

/**********************************
 * 函数名: e
 * 功能: 返回自然对数的底值
 * -------------------------------
 * 输入参数: 无
 * 输出参数: 无
 * -------------------------------
 * @return: 自然对数的底值
 **********************************/
static double e(void)
{
    return 2.71828182845904523536;
}

/**********************************
 * 函数名: fac
 * 功能: 计算一个数的阶乘
 * -------------------------------
 * 输入参数: a - 数值
 * 输出参数: 无
 * -------------------------------
 * @return: a 的阶乘值
 **********************************/
static double fac(double a)
{/* simplest version of fac */
    if (a < 0.0)
    {
        return NAN;
    }

    if (a > UINT_MAX)
    {
        return INFINITY;
    }

    unsigned int ua = (unsigned int) (a);
    unsigned long int result = 1, i;
    for (i = 1; i <= ua; i++)
    {
        if (i > ULONG_MAX / result)
            return INFINITY;
        result *= i;
    }
    return (double) result;
}

/**********************************
 * 函数名: ncr
 * 功能: 计算组合数 nCr
 * -------------------------------
 * 输入参数: n - 总数
 *          r - 取出数
 * 输出参数: 无
 * -------------------------------
 * @return: 组合数 nCr 的值
 **********************************/
static double ncr(double n, double r)
{
    if (n < 0.0 || r < 0.0 || n < r)
        return NAN;
    if (n > UINT_MAX || r > UINT_MAX)
        return INFINITY;
    unsigned long int un = (unsigned int) (n), ur = (unsigned int) (r), i;
    unsigned long int result = 1;
    if (ur > un / 2)
        ur = un - ur;
    for (i = 1; i <= ur; i++)
    {
        if (result > ULONG_MAX / (un - ur + i))
            return INFINITY;
        result *= un - ur + i;
        result /= i;
    }
    return result;
}

/**********************************
 * 函数名: npr
 * 功能: 计算排列数 nPr
 * -------------------------------
 * 输入参数: n - 总数
 *          r - 取出数
 * 输出参数: 无
 * -------------------------------
 * @return: 排列数 nPr 的值
 **********************************/
static double npr(double n, double r)
{
    return ncr(n, r) * fac(r);
}

/**************************************
 * Built-in functions
 * must be in alphabetical order
 **************************************/
typedef struct {
    const char *name;       // function name
    const void *funcPtr;    // function pointer
    int arity;              // number of arguments
} funcType_s;

static const funcType_s s_buildInFunctions[] = {
                                                    { "abs", fabs, 1 },
                                                    { "acos", acos, 1 },
                                                    { "asin", asin, 1 },
                                                    { "atan", atan, 1 },
                                                    { "atan2", atan2, 2 },
                                                    { "ceil", ceil, 1 },
                                                    { "cos", cos, 1 },
                                                    { "cosh", cosh, 1 },
                                                    { "e", e, 0 },
                                                    { "exp", exp, 1 },
                                                    { "fac", fac, 1 },
                                                    { "floor", floor, 1 },
                                                    { "ln", log, 1 },
                                                    { "log", log, 1 },
                                                    { "log10", log10, 1 },
                                                    { "ncr", ncr, 2 },
                                                    { "npr", npr, 2 },
                                                    { "pi", pi, 0 },
                                                    { "pow", pow, 2 },
                                                    { "sin", sin, 1 },
                                                    { "sinh", sinh, 1 },
                                                    { "sqrt", sqrt, 1 },
                                                    { "tan", tan, 1 },
                                                    { "tanh", tanh, 1 },
                                                    { NULL, NULL, 0 }
};

double max(double a, double b)
{
    return a > b ? a : b;
}

double min(double a, double b)
{
    return a < b ? a : b;
}

// 自定义的函数
static funcType_s customFunctions[] = {
                                        { "max", max, 2 },
                                        { "min", min, 2 },  //按需向下面扩展函数
};

/***************************************************
 * 函数名: findBuilDIn
 * 功能: 查找内置函数
 * --------------------------------------------------
 * 输入参数: name - 函数名
 *          len - 函数名长度
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 找到返回函数指针, 否则返回NULL
 ****************************************************/
static const funcType_s* findBuilDIn(const char *name, int len)
{
    int imin = 0;
    int imax = sizeof(s_buildInFunctions) / sizeof(funcType_s) - 2;

    // binary search
    while (imax >= imin)
    {
        const int i = (imin + ((imax - imin) / 2));
        int c = strncmp(name, s_buildInFunctions[i].name, len);
        if (!c)
        {
            c = '\0' - s_buildInFunctions[i].name[len];
        }

        if (c == 0)
        {
            return s_buildInFunctions + i;
        }
        else if (c > 0)
        {
            imin = i + 1;
        }
        else
        {
            imax = i - 1;
        }
    }

    return NULL;
}

/***************************************************
 * 函数名: find_customFunction
 * 功能: 查找自定义函数
 * --------------------------------------------------
 * 输入参数: s - 自定义函数数组
 *          lookupLen - 自定义函数数组长度
 *          name - 函数名
 *          len - 函数名长度
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 找到返回函数指针, 否则返回NULL
 ****************************************************/
static const funcType_s* find_customFunction(const funcType_s *s, int lookupLen, const char *name, int len)
{
    if (!s || lookupLen <= 0)
    {
        return NULL;
    }

    for (int i = 0; i < lookupLen; i++)
    {
        if (strncmp(name, s[i].name, len) == 0 && s[i].name[len] == '\0')
        {
            return (s + i);
        }
    }

    return NULL;
}

static ASTNode_s* node_number(double val, int pos)
{
    ASTNode_s *n = malloc(sizeof(ASTNode_s));
    n->type = N_NUMBER;
    n->pos = pos;
    n->v.number = val;
    return n;
}

static ASTNode_s* node_realDataBase(int id, int pos)
{
    ASTNode_s *n = malloc(sizeof(ASTNode_s));
    n->type = N_REAL_DATABASE;
    n->pos = pos;
    n->v.realDataBaseId = id;
    return n;
}

static ASTNode_s* node_unary(UnaryOp op, ASTNode_s *child, int pos)
{
    ASTNode_s *n = malloc(sizeof(ASTNode_s));
    n->type = N_UNARY;
    n->pos = pos;
    n->v.unary.op = op;
    n->v.unary.child = child;
    return n;
}

static ASTNode_s* node_binary(BinaryOp op, ASTNode_s *l, ASTNode_s *r, int pos)
{
    ASTNode_s *n = malloc(sizeof(ASTNode_s));
    n->type = N_BINARY;
    n->pos = pos;
    n->v.binary.op = op;
    n->v.binary.left = l;
    n->v.binary.right = r;
    return n;
}

static ASTNode_s* node_func(const char *name, ASTNode_s **args, int argc, int pos, void *funcPtr)
{
    ASTNode_s *n = malloc(sizeof(ASTNode_s));
    n->type = N_FUNC;
    n->pos = pos;
    n->v.func.name = strdup(name);
    n->v.func.args = args;
    n->v.func.argc = argc;
    n->v.func.funcPtr = funcPtr;

    return n;
}

static ASTNode_s* node_assign(int id, ASTNode_s *rhs, int pos)
{
    ASTNode_s *n = malloc(sizeof(ASTNode_s));
    n->type = N_ASSIGN;
    n->pos = pos;
    n->v.assign.id = id;
    n->v.assign.rhs = rhs;
    return n;
}

static void free_node(ASTNode_s *n)
{
    if (!n)
    {
        return;
    }

    switch (n->type)
    {
        case N_NUMBER:
            break;
        case N_REAL_DATABASE:
            break;
        case N_UNARY:
            free_node(n->v.unary.child);
            break;
        case N_BINARY:
            free_node(n->v.binary.left);
            free_node(n->v.binary.right);
            break;
        case N_FUNC:
            free(n->v.func.name);
            if (n->v.func.args)
            {
                for (int i = 0; i < n->v.func.argc; ++i)
                    free_node(n->v.func.args[i]);
                free(n->v.func.args);
            }
            break;
        case N_ASSIGN:
            free_node(n->v.assign.rhs);
            break;
    }

    free(n);
}

// printing AST
static void print_node(ASTNode_s *n, const char *indent, int last)
{
    if (!n)
    {
        return;
    }

    printf("%s%s", indent, last ? "└─ " : "├─ ");
    switch (n->type)
    {
        case N_NUMBER:
            printf("%g\n", n->v.number);
            break;
        case N_REAL_DATABASE:
            printf("#%d\n", n->v.realDataBaseId);
            break;
        case N_UNARY:
            printf("Unary(%s)\n", n->v.unary.op == U_NEG ? "-" : (n->v.unary.op == U_NOT ? "!" : "~"));
            {
                char buf[256];
                snprintf(buf, sizeof(buf), "%s%s", indent, last ? "   " : "│  ");
                print_node(n->v.unary.child, buf, 1);
            }
            break;
        case N_BINARY:
        {
            const char *name = "?";
            switch (n->v.binary.op)
            {
                case B_ADD:
                    name = "+";
                    break;
                case B_SUB:
                    name = "-";
                    break;
                case B_MUL:
                    name = "*";
                    break;
                case B_DIV:
                    name = "/";
                    break;
                case B_LSHIFT:
                    name = "<<";
                    break;
                case B_RSHIFT:
                    name = ">>";
                    break;
                case B_GT:
                    name = ">";
                    break;
                case B_GTE:
                    name = ">=";
                    break;
                case B_LT:
                    name = "<";
                    break;
                case B_LTE:
                    name = "<=";
                    break;
                case B_EQ:
                    name = "==";
                    break;
                case B_NEQ:
                    name = "!=";
                    break;
                case B_BITAND:
                    name = "&";
                    break;
                case B_BITXOR:
                    name = "^";
                    break;
                case B_BITOR:
                    name = "|";
                    break;
                case B_ANDAND:
                    name = "&&";
                    break;
                case B_OROR:
                    name = "||";
                    break;
            }

            printf("Binary(%s)\n", name);
            char buf[256];
            snprintf(buf, sizeof(buf), "%s%s", indent, last ? "   " : "│  ");
            print_node(n->v.binary.left, buf, 0);
            print_node(n->v.binary.right, buf, 1);
        }
            break;
        case N_FUNC:
            printf("Func(%s)\n", n->v.func.name);
            {
                char buf[256];
                snprintf(buf, sizeof(buf), "%s%s", indent, last ? "   " : "│  ");
                for (int i = 0; i < n->v.func.argc; ++i)
                {
                    print_node(n->v.func.args[i], buf, i == n->v.func.argc - 1);
                }
            }
            break;
        case N_ASSIGN:
            printf("Assign(#%d)\n", n->v.assign.id);
            {
                char buf[256];
                snprintf(buf, sizeof(buf), "%s%s", indent, last ? "   " : "│  ");
                print_node(n->v.assign.rhs, buf, 1);
            }
            break;
    }
}

/***********************************************************
 * 函数名: eval_node
 * 功能: 计算AST节点的值. 支持短路求值.
 * "evaluation with short-circuit" means
 * 1. Short-circuit evaluation for logical operators means
 *    the right-hand operand is only evaluated when needed
 *    to determine the result.
 * 2. For logical AND (&&): if the left operand is
 *    false (here, 0.0), the whole expression is false and
 *    the right operand is NOT evaluated.
 * 3. For logical OR (||): if the left operand is
 *    true (non-zero), the whole expression is true and
 *    the right operand is NOT evaluated.
 * 4. This prevents unnecessary work and avoids side
 *    effects or runtime errors that would occur
 *    if the right-hand side were evaluated.
 * ---------------------------------------------------------
 * 输入参数: n - AST节点
 * 输入参数: rt - 实时库
 * ---------------------------------------------------------
 * @return: 计算结果
 ***********************************************************/
static double eval_node(ASTNode_s *n)
{
    if (!n)
    {
        return 0.0;
    }

    switch (n->type)
    {
        case N_NUMBER:
            return n->v.number;
        case N_REAL_DATABASE:
            return rt_get(n->v.realDataBaseId);
        case N_UNARY:
        {
            double v = eval_node(n->v.unary.child);
            if (n->v.unary.op == U_NEG)
            {
                return -v;
            }

            if (n->v.unary.op == U_NOT)
            {
                return v != 0.0 ? 0.0 : 1.0;
            }

            return (double) (~((long) v));
        }
        case N_BINARY:
        {
            switch (n->v.binary.op)
            {
                case B_ADD:
                    return eval_node(n->v.binary.left) + eval_node(n->v.binary.right);
                case B_SUB:
                    return eval_node(n->v.binary.left) - eval_node(n->v.binary.right);
                case B_MUL:
                    return eval_node(n->v.binary.left) * eval_node(n->v.binary.right);
                case B_DIV:
                {
                    double r = eval_node(n->v.binary.right);
                    if (r == 0)
                    {
                        fprintf(stderr, "Runtime error: division by zero at pos %d\n", n->pos);
                        exit(1);
                    }

                    return eval_node(n->v.binary.left) / r;
                }
                case B_LSHIFT:
                    return (double) (((long) eval_node(n->v.binary.left)) << (int) eval_node(n->v.binary.right));
                case B_RSHIFT:
                    return (double) (((long) eval_node(n->v.binary.left)) >> (int) eval_node(n->v.binary.right));
                case B_GT:
                    return eval_node(n->v.binary.left) > eval_node(n->v.binary.right) ? 1.0 : 0.0;
                case B_GTE:
                    return eval_node(n->v.binary.left) >= eval_node(n->v.binary.right) ? 1.0 : 0.0;
                case B_LT:
                    return eval_node(n->v.binary.left) < eval_node(n->v.binary.right) ? 1.0 : 0.0;
                case B_LTE:
                    return eval_node(n->v.binary.left) <= eval_node(n->v.binary.right) ? 1.0 : 0.0;
                case B_EQ:
                    return eval_node(n->v.binary.left) == eval_node(n->v.binary.right) ? 1.0 : 0.0;
                case B_NEQ:
                    return eval_node(n->v.binary.left) != eval_node(n->v.binary.right) ? 1.0 : 0.0;
                case B_BITAND:
                    return (double) (((long) eval_node(n->v.binary.left)) & ((long) eval_node(n->v.binary.right)));
                case B_BITXOR:
                    return (double) (((long) eval_node(n->v.binary.left)) ^ ((long) eval_node(n->v.binary.right)));
                case B_BITOR:
                    return (double) (((long) eval_node(n->v.binary.left)) | ((long) eval_node(n->v.binary.right)));
                case B_ANDAND:
                {
                    double lv = eval_node(n->v.binary.left);
                    if (EVAL_USE_SHORT_CIRCUIT && lv == 0.0)
                    {
                        return 0.0;
                    }

                    double rv = eval_node(n->v.binary.right);
                    return (lv == 0.0 || rv == 0.0) ? 0.0 : 1.0;
                }
                case B_OROR:
                {
                    double lv = eval_node(n->v.binary.left);
                    if (EVAL_USE_SHORT_CIRCUIT && lv != 0.0)
                    {
                        return 1.0;
                    }

                    double rv = eval_node(n->v.binary.right);
                    return (lv == 0.0 && rv == 0.0) ? 0.0 : 1.0;
                }
            }
            break;
        }
        case N_FUNC:
        {
            // evaluate args
            double args_vals[4];
            for (int i = 0; i < n->v.func.argc; ++i)
            {
                if (i < 4)
                {
                    args_vals[i] = eval_node(n->v.func.args[i]);
                }
            }

            // dispatch based on arity
            if (!n->v.func.funcPtr)
            {
                fprintf(stderr, "Runtime error: unknown function %s at pos %d\n", n->v.func.name, n->pos);
                exit(1);
            }

            // support up to 2-arg functions; constants like pi/e have argc==0
            if (n->v.func.argc == 0)
            {
                double (*f0)(void) = (double (*)(void))n->v.func.funcPtr;
                return f0();
            }
            else if (n->v.func.argc == 1)
            {
                double (*f1)(double) = (double (*)(double))n->v.func.funcPtr;

                if(f1 == sin || f1 == cos || f1 == tan)
                {
                    return f1((double)(args_vals[0] * pi() / 180.0));
                }

                if(f1 == asin || f1 == acos || f1 == atan)
                {
                    return f1(args_vals[0]) * 180.0 / pi();
                }

                return f1(args_vals[0]);
            }
            else if (n->v.func.argc == 2)
            {
                double (*f2)(double, double) = (double (*)(double, double))n->v.func.funcPtr;
                return f2(args_vals[0], args_vals[1]);
            }
            else
            {
                fprintf(stderr, "Runtime error: function %s with arity %d not supported at pos %d\n", n->v.func.name, n->v.func.argc, n->pos);
                exit(1);
            }
        }
        case N_ASSIGN:
        {
            double v = eval_node(n->v.assign.rhs);
            rt_set(n->v.assign.id, v);
            return v;
        }
    }

    return 0.0;
}

/*-------------------------------------------------- Parser functions follow grammar and precedence --------------------------------------------------*/

// Forward declarations
static ASTNode_s* parse_assign(TokenList *toks);
static ASTNode_s* parse_logical_or_node(TokenList *toks);
static ASTNode_s* parse_logical_and_node(TokenList *toks);
static ASTNode_s* parse_bitor_node(TokenList *toks);
static ASTNode_s* parse_bitxor_node(TokenList *toks);
static ASTNode_s* parse_bitand_node(TokenList *toks);
static ASTNode_s* parse_equality_node(TokenList *toks);
static ASTNode_s* parse_relational_node(TokenList *toks);
static ASTNode_s* parse_shift_node(TokenList *toks);
static ASTNode_s* parse_add_node(TokenList *toks);
static ASTNode_s* parse_multiply_node(TokenList *toks);
static ASTNode_s* parse_unary_node(TokenList *toks);
static ASTNode_s* parse_power_node(TokenList *toks);
static ASTNode_s* parse_primary_node(TokenList *toks);

/***************************************
 * 函数名: match
 * 功能: 如果下一个Token类型匹配则消耗掉它
 * -------------------------------------
 * 输入参数: t - Token列表
 *          ty - 期望的Token类型
 * 输出参数: 无
 * -------------------------------------
 * @return: 匹配成功返回1, 否则返回0
 **************************************/
static int match(TokenList *t, TokenType ty)
{
    if (tlist_peek(t).type == ty)
    {
        tlist_next(t);
        return 1;
    }

    return 0;
}

/***************************************************
 * 函数名: parse_assign
 * 功能: 解析赋值表达式
 * 语法: assign := REALDB '=' assign | logical_or
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 **************************************************/
static ASTNode_s* parse_assign(TokenList *toks)
{
    Token cur = tlist_peek(toks);
    if (cur.type == T_REALDB &&
            toks->idx + 1 < toks->sz &&
            toks->arr[toks->idx + 1].type == T_ASSIGN)
    {
        Token h = tlist_next(toks); // consume REAL_DATABASE
        Token a = tlist_next(toks); // consume ASSIGN
        ASTNode_s *rhs = parse_assign(toks); // right-assoc
        int id = atoi(h.text);
        return node_assign(id, rhs, a.pos);
    }

    return parse_logical_or_node(toks);
}

/***************************************************
 * 函数名: parse_logical_or_node
 * 功能: 解析逻辑或表达式
 * 语法: logical_or := logical_and ('||' logical_and)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 **************************************************/
static ASTNode_s* parse_logical_or_node(TokenList *toks)
{
    ASTNode_s *left = parse_logical_and_node(toks);
    while (match(toks, T_OROR))
    {
        ASTNode_s *right = parse_logical_and_node(toks);
        left = node_binary(B_OROR, left, right, left->pos);
    }

    return left;
}

/***************************************************
 * 函数名: parse_logical_and_node
 * 功能: 解析逻辑与表达式
 * 语法: logical_and := bitor ('&&' bitor)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 **************************************************/
static ASTNode_s* parse_logical_and_node(TokenList *toks)
{
    ASTNode_s *left = parse_bitor_node(toks);
    while (match(toks, T_ANDAND))
    {
        ASTNode_s *right = parse_bitor_node(toks);
        left = node_binary(B_ANDAND, left, right, left->pos);
    }

    return left;
}

/***************************************************
 * 函数名: parse_bitor_node
 * 功能: 解析按位或表达式
 * 语法: bitor := bitxor ('|' bitxor)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ***************************************************/
static ASTNode_s* parse_bitor_node(TokenList *toks)
{
    ASTNode_s *left = parse_bitxor_node(toks);
    while (match(toks, T_PIPE))
    {
        ASTNode_s *r = parse_bitxor_node(toks);
        left = node_binary(B_BITOR, left, r, left->pos);
    }

    return left;
}

/***************************************************
 * 函数名: parse_bitxor_node
 * 功能: 解析按位异或表达式
 * 语法: bitxor := bitand ('^' bitand)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_bitxor_node(TokenList *toks)
{
    ASTNode_s *left = parse_bitand_node(toks);
    while (match(toks, T_CARET))
    {
        ASTNode_s *r = parse_bitand_node(toks);
        left = node_binary(B_BITXOR, left, r, left->pos);
    }

    return left;
}

/***************************************************
 * 函数名: parse_bitand_node
 * 功能: 解析按位与表达式
 * 语法: bitand := equality ('&' equality)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_bitand_node(TokenList *toks)
{
    ASTNode_s *left = parse_equality_node(toks);
    while (match(toks, T_AMP))
    {
        ASTNode_s *r = parse_equality_node(toks);
        left = node_binary(B_BITAND, left, r, left->pos);
    }

    return left;
}

/***************************************************
 * 函数名: parse_equality_node
 * 功能: 解析相等/不等表达式
 * 语法: equality := relational (('==' | '!=') relational)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_equality_node(TokenList *toks)
{
    ASTNode_s *left = parse_relational_node(toks);
    while (1)
    {
        if (match(toks, T_EQ))
        {
            ASTNode_s *r = parse_relational_node(toks);
            left = node_binary(B_EQ, left, r, left->pos);
        }
        else if (match(toks, T_NEQ))
        {
            ASTNode_s *r = parse_relational_node(toks);
            left = node_binary(B_NEQ, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

/***************************************************
 * 函数名: parse_relational_node
 * 功能: 解析关系表达式
 * 语法: relational := shift (('>' | '>=' | '<' | '<=') shift)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_relational_node(TokenList *toks)
{
    ASTNode_s *left = parse_shift_node(toks);
    while (1)
    {
        if (match(toks, T_GT))
        {
            ASTNode_s *r = parse_shift_node(toks);
            left = node_binary(B_GT, left, r, left->pos);
        }
        else if (match(toks, T_GTE))
        {
            ASTNode_s *r = parse_shift_node(toks);
            left = node_binary(B_GTE, left, r, left->pos);
        }
        else if (match(toks, T_LT))
        {
            ASTNode_s *r = parse_shift_node(toks);
            left = node_binary(B_LT, left, r, left->pos);
        }
        else if (match(toks, T_LTE))
        {
            ASTNode_s *r = parse_shift_node(toks);
            left = node_binary(B_LTE, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

/***************************************************
 * 函数名: parse_shift_node
 * 功能: 解析移位表达式
 * 语法: shift := add (('<<' | '>>') add)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_shift_node(TokenList *toks)
{
    ASTNode_s *left = parse_add_node(toks);
    while (1)
    {
        if (match(toks, T_LSHIFT))
        {
            ASTNode_s *r = parse_add_node(toks);
            left = node_binary(B_LSHIFT, left, r, left->pos);
        }
        else if (match(toks, T_RSHIFT))
        {
            ASTNode_s *r = parse_add_node(toks);
            left = node_binary(B_RSHIFT, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

/***************************************************
 * 函数名: parse_add_node
 * 功能: 解析加减表达式
 * 语法: add := multiply (('+' | '-') multiply)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_add_node(TokenList *toks)
{
    ASTNode_s *left = parse_multiply_node(toks);
    while (1)
    {
        if (match(toks, T_PLUS))
        {
            ASTNode_s *r = parse_multiply_node(toks);
            left = node_binary(B_ADD, left, r, left->pos);
        }
        else if (match(toks, T_MINUS))
        {
            ASTNode_s *r = parse_multiply_node(toks);
            left = node_binary(B_SUB, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

/***************************************************
 * 函数名: parse_multiply_node
 * 功能: 解析乘除表达式
 * 语法: multiply := unary (('*' | '/') unary)*
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_multiply_node(TokenList *toks)
{
    ASTNode_s *left = parse_unary_node(toks);
    while (1)
    {
        if (match(toks, T_MUL))
        {
            ASTNode_s *r = parse_unary_node(toks);
            left = node_binary(B_MUL, left, r, left->pos);
        }
        else if (match(toks, T_DIV))
        {
            ASTNode_s *r = parse_unary_node(toks);
            left = node_binary(B_DIV, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

/***************************************************
 * 函数名: parse_unary_node
 * 功能: 解析一元表达式
 * 语法: unary := ('!' | '~' | '-') unary | power
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_unary_node(TokenList *toks)
{
    if (match(toks, T_NOT))
    {
        ASTNode_s *op = parse_unary_node(toks);
        return node_unary(U_NOT, op, op->pos);
    }

    if (match(toks, T_TILDE))
    {
        ASTNode_s *op = parse_unary_node(toks);
        return node_unary(U_BITNOT, op, op->pos);
    }

    if (match(toks, T_MINUS))
    {
        ASTNode_s *op = parse_unary_node(toks);
        return node_unary(U_NEG, op, op->pos);
    }

    return parse_power_node(toks);
}

/***************************************************
 * 函数名: parse_power_node
 * 功能: 解析函数调用或基础表达式
 * 语法: power := IDENT '(' (assign (',' assign)*)? ')' | primary
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_power_node(TokenList *toks)
{
    Token cur = tlist_peek(toks);

    const funcType_s *func = findBuilDIn(cur.text, (int) strlen(cur.text));

    if (!func)
    {
        int customFunctionCount = sizeof(customFunctions) / sizeof(customFunctions[0]);
        func = find_customFunction(customFunctions, customFunctionCount, cur.text, (int) strlen(cur.text));
    }

    if (cur.type == T_IDENT && func != NULL)
    {
        tlist_next(toks);
        if (!match(toks, T_LP))
        {
            fprintf(stderr, "Syntax error: expected '(' after %s at %d\n", cur.text, cur.pos);
            exit(1);
        }
        // parse argument list (comma separated)
        ASTNode_s **args = NULL;
        int argc = 0;
        if (!match(toks, T_RP))
        {
            while (1)
            {
                ASTNode_s *a = parse_assign(toks);
                args = realloc(args, sizeof(ASTNode_s*) * (argc + 1));
                args[argc++] = a;
                if (match(toks, T_RP))
                    break;
                if (!match(toks, T_COMMA))
                {
                    fprintf(stderr, "Syntax error: expected ',' or ')' after %s at %d\n", cur.text, cur.pos);
                    exit(1);
                }
            }
        }
        // validate arity
        if (func->arity >= 0 && func->arity != argc)
        {
            fprintf(stderr, "Syntax error: function %s expects %d args, got %d at %d\n", cur.text, func->arity, argc, cur.pos);
            exit(1);
        }

        ASTNode_s *fn = node_func(cur.text, args, argc, cur.pos, (void*) func->funcPtr);
        return fn;
    }

    return parse_primary_node(toks);
}

/***************************************************
 * 函数名: parse_primary_node
 * 功能: 解析基础表达式
 * 语法: primary := NUM | REALDB | '(' assign ')'
 * --------------------------------------------------
 * 输入参数: toks - Token列表
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 解析得到的AST节点
 ****************************************************/
static ASTNode_s* parse_primary_node(TokenList *toks)
{
    Token t = tlist_peek(toks);
    if (t.type == T_NUM)
    {
        Token tk = tlist_next(toks);
        return node_number(tk.num, tk.pos);
    }

    if (t.type == T_REALDB)
    {
        Token tk = tlist_next(toks);
        int id = atoi(tk.text);
        return node_realDataBase(id, tk.pos);
    }

    if (t.type == T_IDENT)
    {
        fprintf(stderr, "Syntax error: unexpected identifier '%s' at %d\n", t.text, t.pos);
        exit(1);
    }

    if (match(toks, T_LP))
    {
        ASTNode_s *v = parse_assign(toks);
        Token r = tlist_peek(toks);
        if (!match(toks, T_RP))
        {
            fprintf(stderr, "Syntax error: expected ')' at %d\n", r.pos);
            exit(1);
        }
        return v;
    }

    fprintf(stderr, "Syntax error: unexpected token at pos %d\n", t.pos);
    exit(1);
}

/***************************************************
 * 函数名: tokenize
 * 功能: 将输入字符串分解为Token列表
 * --------------------------------------------------
 * 输入参数: s - 输入字符串
 * 输出参数: out - 输出Token列表
 * --------------------------------------------------
 * @return: 无
 ****************************************************/
static void tokenize(const char *s, TokenList *out)
{
    int i = 0;
    int n = (int) strlen(s);
    while (1)
    {
        while (i < n && isspace((unsigned char )s[i]))
        {
            i++;
        }

        if (i >= n)
        {
            Token t = { T_EOF, NULL, 0, i };
            tlist_push(out, t);
            break;
        }

        char c = s[i];
        // multi-char tokens
        if (c == '&' && i + 1 < n && s[i + 1] == '&')
        {
            Token t = { T_ANDAND, strdup("&&"), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '|' && i + 1 < n && s[i + 1] == '|')
        {
            Token t = { T_OROR, strdup("||"), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '<' && i + 1 < n && s[i + 1] == '<')
        {
            Token t = { T_LSHIFT, strdup("<<"), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '>' && i + 1 < n && s[i + 1] == '>')
        {
            Token t = { T_RSHIFT, strdup(">>"), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '>' && i + 1 < n && s[i + 1] == '=')
        {
            Token t = { T_GTE, strdup(">="), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '<' && i + 1 < n && s[i + 1] == '=')
        {
            Token t = { T_LTE, strdup("<="), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '!' && i + 1 < n && s[i + 1] == '=')
        {
            Token t = { T_NEQ, strdup("!="), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        if (c == '=' && i + 1 < n && s[i + 1] == '=')
        {
            Token t = { T_EQ, strdup("=="), 0, i };
            tlist_push(out, t);
            i += 2;
            continue;
        }

        // numbers: must start with a digit. If '.' present, it must be followed by one or more digits.
        if (isdigit((unsigned char )c))
        {
            int start = i;
            // integer part
            while (i < n && isdigit((unsigned char )s[i]))
                i++;
            // optional fractional part: only accept if '.' followed by at least one digit
            if (i < n && s[i] == '.')
            {
                int dot = i;
                if (i + 1 < n && isdigit((unsigned char )s[i + 1]))
                {
                    // consume '.' and fractional digits
                    i++; // consume '.'
                    while (i < n && isdigit((unsigned char )s[i]))
                        i++;
                }
                else
                {
                    // '.' not followed by digit: do not consume it as part of number
                    i = dot; // rewind to dot so it will be processed later
                }
            }
            int len = i - start;
            char *txt = strndup(s + start, len);
            double v = strtod(txt, NULL);
            Token t = { T_NUM, txt, v, start };
            tlist_push(out, t);
            continue;
        }

        if (c == '#')
        {
            i++;
            int start = i;
            while (i < n && isdigit((unsigned char )s[i]))
                i++;
            if (start == i)
            {
                Token t = { T_INVALID, NULL, 0, start - 1 };
                tlist_push(out, t);
                break;
            }
            int len = i - start;
            char *txt = strndup(s + start, len);
            Token t = { T_REALDB, txt, 0, start - 1 };
            tlist_push(out, t);
            continue;
        }

        // identifiers: must start with a letter or underscore, followed by letters, digits or underscores
        if (isalpha((unsigned char)c) || c == '_')
        {
            int start = i;
            i++; // consume first
            while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '_'))
                i++;
            int len = i - start;
            char *txt = strndup(s + start, len);
            Token t = { T_IDENT, txt, 0, start };
            tlist_push(out, t);
            continue;
        }

        // single char
        switch (c)
        {
            case '+':
            {
                Token t = { T_PLUS, strdup("+"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '-':
            {
                Token t = { T_MINUS, strdup("-"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '*':
            {
                Token t = { T_MUL, strdup("*"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '/':
            {
                Token t = { T_DIV, strdup("/"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '(':
            {
                Token t = { T_LP, strdup("("), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case ')':
            {
                Token t = { T_RP, strdup(")"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '!':
            {
                Token t = { T_NOT, strdup("!"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '>':
            {
                Token t = { T_GT, strdup(">"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '<':
            {
                Token t = { T_LT, strdup("<"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '&':
            {
                Token t = { T_AMP, strdup("&"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '|':
            {
                Token t = { T_PIPE, strdup("|"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '^':
            {
                Token t = { T_CARET, strdup("^"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '~':
            {
                Token t = { T_TILDE, strdup("~"), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case '=':
            {
                Token t = { T_ASSIGN, strdup("="), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            case ',':
            {
                Token t = { T_COMMA, strdup(","), 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
            default:
            {
                Token t = { T_INVALID, NULL, 0, i };
                tlist_push(out, t);
                i++;
                break;
            }
        }
    }
}

/*****************************************************
 * 函数名: print_error_with_caret
 * 功能: 打印错误信息并在指定位置显示插入符号
 * --------------------------------------------------
 * 输入参数: line - 输入字符串
 *          pos - 错误位置
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 无
 ****************************************************/
static void print_error_with_caret(const char *line, int pos)
{
    fprintf(stderr, "%s\n", line);
    for (int i = 0; i < pos && line[i]; i++)
    {
        fputc(line[i] == '\t' ? '\t' : ' ', stderr);
    }

    fprintf(stderr, "^\n");
}

/*****************************************************
 * 函数名: node_contains_realDatabaseId
 * 功能: 检查节点子树中是否包含真实数据库ID节点
 * Optimization: constant-fold subtrees that do not
 * contain any real database id (#id).
 * Real database nodes (N_REAL_DATABASE) must not be
 * folded because their values may change concurrently.
 * Return 1 if subtree contains a real database node,
 *        0 otherwise
 * --------------------------------------------------
 * 输入参数: n - AST节点
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 包含真实数据库ID节点返回1, 否则返回0
 ****************************************************/
static int node_contains_realDatabaseId(ASTNode_s *n)
{
    if (!n)
    {
        return 0;
    }

    switch (n->type)
    {
        case N_REAL_DATABASE:
            return 1;
        case N_NUMBER:
            return 0;
        case N_UNARY:
            return node_contains_realDatabaseId(n->v.unary.child);
        case N_BINARY:
            return node_contains_realDatabaseId(n->v.binary.left) || node_contains_realDatabaseId(n->v.binary.right);
        case N_FUNC:
        {
            if (!n->v.func.args)
                return 0;
            for (int i = 0; i < n->v.func.argc; ++i)
            {
                if (node_contains_realDatabaseId(n->v.func.args[i]))
                    return 1;
            }
            return 0;
        }
        case N_ASSIGN:
            return node_contains_realDatabaseId(n->v.assign.rhs); // assignment lhs is id, not a real database node
        default:
            return 0;
    }
}

/******************************************************
 * 函数名: node_get_number
 * 功能: 获取数字节点的值
 * Helper to get number from a node
 * (assumes node->type == N_NUMBER)
 * --------------------------------------------------
 * 输入参数: n - AST节点
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 数字节点的值
 ****************************************************/
static double node_get_number(ASTNode_s *n)
{
    return n->v.number;
}

/******************************************************
 * 函数名: optimize_node
 * 功能: 优化AST节点
 * Constant-folding optimizer;
 * returns possibly new node
 * (caller must use returned pointer).
 * --------------------------------------------------
 * 输入参数: n - AST节点
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 优化后的AST节点
 ****************************************************/
static ASTNode_s* optimize_node(ASTNode_s *n)
{
    if (!n)
    {
        return NULL;
    }

    switch (n->type)
    {
        case N_NUMBER:
            case N_REAL_DATABASE:
            return n;

        case N_UNARY:
        {
            n->v.unary.child = optimize_node(n->v.unary.child);
            if (!node_contains_realDatabaseId(n) && n->v.unary.child && n->v.unary.child->type == N_NUMBER)
            {
                double c = node_get_number(n->v.unary.child);
                double res;
                if (n->v.unary.op == U_NEG)
                    res = -c;
                else if (n->v.unary.op == U_NOT)
                    res = (c != 0.0) ? 0.0 : 1.0;
                else
                    /* U_BITNOT */res = (double) (~((long) c));
                int pos = n->pos;
                free_node(n);
                return node_number(res, pos);
            }

            return n;
        }

        case N_BINARY:
        {
            n->v.binary.left = optimize_node(n->v.binary.left);
            n->v.binary.right = optimize_node(n->v.binary.right);
            if (!node_contains_realDatabaseId(n) && n->v.binary.left && n->v.binary.right
                    && n->v.binary.left->type == N_NUMBER && n->v.binary.right->type == N_NUMBER)
            {
                double l = node_get_number(n->v.binary.left);
                double r = node_get_number(n->v.binary.right);
                double res = 0.0;
                int can_fold = 1;
                switch (n->v.binary.op)
                {
                    case B_ADD:
                        res = l + r;
                        break;
                    case B_SUB:
                        res = l - r;
                        break;
                    case B_MUL:
                        res = l * r;
                        break;
                    case B_DIV:
                        if (r == 0.0)
                            can_fold = 0;
                        else
                            res = l / r;
                        break;
                    case B_LSHIFT:
                        res = (double) (((long) l) << (int) r);
                        break;
                    case B_RSHIFT:
                        res = (double) (((long) l) >> (int) r);
                        break;
                    case B_GT:
                        res = l > r ? 1.0 : 0.0;
                        break;
                    case B_GTE:
                        res = l >= r ? 1.0 : 0.0;
                        break;
                    case B_LT:
                        res = l < r ? 1.0 : 0.0;
                        break;
                    case B_LTE:
                        res = l <= r ? 1.0 : 0.0;
                        break;
                    case B_EQ:
                        res = l == r ? 1.0 : 0.0;
                        break;
                    case B_NEQ:
                        res = l != r ? 1.0 : 0.0;
                        break;
                    case B_BITAND:
                        res = (double) (((long) l) & ((long) r));
                        break;
                    case B_BITXOR:
                        res = (double) (((long) l) ^ ((long) r));
                        break;
                    case B_BITOR:
                        res = (double) (((long) l) | ((long) r));
                        break;
                    case B_ANDAND:
                        res = (l == 0.0) ? 0.0 : (r != 0.0 ? 1.0 : 0.0);
                        break;
                    case B_OROR:
                        res = (l != 0.0) ? 1.0 : (r != 0.0 ? 1.0 : 0.0);
                        break;
                    default:
                        can_fold = 0;
                        break;
                }

                if (can_fold)
                {
                    int pos = n->pos;
                    free_node(n);
                    return node_number(res, pos);
                }
            }

            return n;
        }

        case N_FUNC:
        {
            /* optimize each argument */
            for (int i = 0; i < n->v.func.argc; ++i)
            {
                n->v.func.args[i] = optimize_node(n->v.func.args[i]);
            }

            /* if subtree contains no real database nodes and all args are numbers, constant-fold */
            if (!node_contains_realDatabaseId(n))
            {
                int all_number = 1;
                for (int i = 0; i < n->v.func.argc; ++i)
                {
                    if (!n->v.func.args[i] || n->v.func.args[i]->type != N_NUMBER)
                    {
                        all_number = 0;
                        break;
                    }
                }

                if (all_number)
                {
                    double vals[4] = { 0.0, 0.0, 0.0, 0.0 };
                    for (int i = 0; i < n->v.func.argc && i < 4; ++i)
                        vals[i] = node_get_number(n->v.func.args[i]);

                    if (n->v.func.funcPtr)
                    {
                        double res = 0.0;
                        if (n->v.func.argc == 0)
                        {
                            double (*f0)(void) = (double (*)(void))n->v.func.funcPtr;
                            res = f0();
                        }
                        else if (n->v.func.argc == 1)
                        {
                            double (*f1)(double) = (double (*)(double))n->v.func.funcPtr;

                            if(f1 == sin || f1 == cos || f1 == tan)
                            {
                                res = f1((double)(vals[0] * pi() / 180.0));
                            }
                            else if(f1 == asin || f1 == acos || f1 == atan)
                            {
                                res = f1(vals[0]) * 180.0 / pi();
                            }
                            else
                            {
                                res = f1(vals[0]);
                            }
                        }
                        else if (n->v.func.argc == 2)
                        {
                            double (*f2)(double, double) = (double (*)(double, double))n->v.func.funcPtr;
                            res = f2(vals[0], vals[1]);
                        }
                        else
                        {
                            return n; /* unsupported arity for folding */
                        }

                        int pos = n->pos;
                        free_node(n);
                        return node_number(res, pos);
                    }
                    else
                    {
                        return n; /* unknown func */
                    }
                }
            }

            return n;
        }

        case N_ASSIGN:
        {
            // do not fold assignment itself (side-effect), but optimize its rhs
            n->v.assign.rhs = optimize_node(n->v.assign.rhs);
            return n;
        }

        default:
            return n;
    }
}

/******************************************************
 * 函数名: eval_main
 * 功能: 主评估循环
 * --------------------------------------------------
 * 输入参数: 无
 * 输出参数: 无
 * --------------------------------------------------
 * @return: 无
 ******************************************************/
void eval_main(void)
{
    char line[8192];
    rt_init(8192);
    printf("expr> ");

    while (fgets(line, sizeof(line), stdin))
    {
        if (line[0] == '\n' || line[0] == 0)
        {
            break;
        }

        TokenList toks;
        tlist_init(&toks);
        // tokenize directly using the tokenizer function above
        // (we already have tokenize implemented earlier, reuse)
        tokenize(line, &toks);
        // find invalid
        int invalid_idx = -1;
        for (int i = 0; i < toks.sz; i++)
        {
            if (toks.arr[i].type == T_INVALID)
            {
                invalid_idx = toks.arr[i].pos;
                break;
            }
        }

        if (invalid_idx >= 0)
        {
            fprintf(stderr, "Lexical error at position %d\n", invalid_idx);
            print_error_with_caret(line, invalid_idx);
            tlist_free(&toks);
            printf("expr> ");
            continue;
        }

        toks.idx = 0;
        ASTNode_s *ast = NULL;

        // parse
        // protect from parse errors with checks
        // using exit on errors inside parser
        ast = parse_assign(&toks);
        Token after = tlist_peek(&toks);
        if (after.type != T_EOF)
        {
            fprintf(stderr, "Syntax error: unexpected token at pos %d\n", after.pos);
            print_error_with_caret(line, after.pos);
            free_node(ast);
            tlist_free(&toks);
            printf("expr> ");
            continue;
        }

        printf("AST:\n");
        print_node(ast, "", 1);

        // optimize
        ast = optimize_node(ast);
        printf("Optimized AST:\n");
        print_node(ast, "", 1);

        // evaluate
        double res = eval_node(ast);
        printf("Result: %g\n", res);
        free_node(ast);
        tlist_free(&toks);
        printf("expr> ");
    }

    rt_free();
}
