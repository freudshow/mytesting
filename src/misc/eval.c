/******************************************************************************************************************
 * grammar eval.g4;
 * ----------------------------------------------------------------------------------------------------------------
 * # Top-level
 * prog : stmt EOF ;
 * stmt : assignStmt | expr ;
 * assignStmt : HASH ASSIGN expr ;
 * ----------------------------------------------------------------------------------------------------------------
 * # Expression entry
 * expr : assignExpr ;
 *
 * # Priority 1 (lowest): assignment, right-associative
 * assignExpr
 *     : logicalOr ( ASSIGN assignExpr )?   # priority 1, right-assoc
 *     ;
 *
 * # Priority 2: || (left-assoc)
 * logicalOr
 *     : logicalAnd ( OROR logicalAnd )*
 *     ;
 *
 * # Priority 3: && (left-assoc)
 * logicalAnd
 *     : bitOr ( ANDAND bitOr )*
 *     ;
 *
 * # Priority 4: bitwise OR '|' (left-assoc)
 * bitOr
 *     : bitXor ( PIPE bitXor )*
 *     ;
 *
 * # Priority 5: bitwise XOR '^' (left-assoc)
 * bitXor
 *     : bitAnd ( CARET bitAnd )*
 *     ;
 *
 * # Priority 6: bitwise AND '&' (left-assoc)
 * bitAnd
 *     : equality ( AMP equality )*
 *     ;
 *
 * # Priority 7: equality '==' '!=' (left-assoc)
 * equality
 *     : relational ( (EQ | NEQ) relational )*
 *     ;
 *
 * # Priority 8: relational < <= > >= (left-assoc)
 * relational
 *     : shift ( (LT | LTE | GT | GTE) shift )*
 *     ;
 *
 * # Priority 9: shifts << >> (left-assoc)
 * shift
 *     : add ( (LSHIFT | RSHIFT) add )*
 *     ;
 *
 * # Priority 10: addition/subtraction + - (left-assoc)
 * add
 *     : mul ( (PLUS | MINUS) mul )*
 *     ;
 *
 * # Priority 11: multiply/divide * / (left-assoc)
 * mul
 *     : unary ( (MULT | DIV) unary )*
 *     ;
 *
 * # Priority 12: unary: ~, !, - (right-assoc)
 * unary
 *     : ( TILDE | NOT | MINUS ) unary
 *     | power
 *     ;
 *
 * # Priority 13: sin, cos function calls (tighter than unary)
 * # handled as primary forms below
 *
 * # Priority 14 (highest): exp function call (tightest)
 * # handled as primary form below
 *
 * # power/primary level (functions and atoms)
 * power
 *     : expFunc                 # ExpFunction
 *     | sinCosFunc              # SinCosFunction
 *     | primary                 # AtomPrimary
 *     ;
 *
 * # function productions
 * expFunc
 *     : EXP LP expr RP          # 'exp(expr)' — priority 14 (highest)
 *     ;
 *
 * sinCosFunc
 *     : ( SIN | COS ) LP expr RP  # 'sin(expr)' or 'cos(expr)' — priority 13
 *     ;
 *
 * # primary atoms
 * primary
 *     : NUMBER
 *     | HASH                     # realtime marker '#123'
 *     | LP expr RP
 *     ;
 *
 * # Lexer tokens (representative)
 * PLUS    : '+' ;
 * MINUS   : '-' ;
 * MULT    : '*' ;
 * DIV     : '/' ;
 * NOT     : '!' ;
 * ANDAND  : '&&' ;
 * OROR    : '||' ;
 * GT      : '>' ;
 * GTE     : '>=' ;
 * LT      : '<' ;
 * LTE     : '<=' ;
 * EQ      : '==' ;
 * NEQ     : '!=' ;
 * AMP     : '&' ;
 * PIPE    : '|' ;
 * CARET   : '^' ;
 * TILDE   : '~' ;
 * LSHIFT  : '<<' ;
 * RSHIFT  : '>>' ;
 * LP      : '(' ;
 * RP      : ')' ;
 * ASSIGN  : '=' ;
 *
 * # functions and identifiers
 * SIN     : 'sin' ;
 * COS     : 'cos' ;
 * EXP     : 'exp' ;
 * NUMBER  : [0-9]+ ('.' [0-9]*)? | '.' [0-9]+ ;
 * HASH    : '#' [0-9]+ ;
 * IDENT   : [a-zA-Z]+ ;
 *
 * # whitespace & error
 * WS      : [ \t\r\n]+ -> skip ;
 * ERROR_CHAR : . -> channel(HIDDEN) ;
 ****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>

typedef enum {
    T_NUM,
    T_HASH,
    T_IDENT,
    T_PLUS,
    T_MINUS,
    T_MUL,
    T_DIV,
    T_LP,
    T_RP,
    T_NOT,
    T_NEQ,
    T_ANDAND,
    T_OROR,
    T_GT,
    T_GTE,
    T_LT,
    T_LTE,
    T_EQ,
    T_AMP,
    T_PIPE,
    T_CARET,
    T_TILDE,
    T_LSHIFT,
    T_RSHIFT,
    T_ASSIGN,
    T_EOF, T_INVALID
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

static Token tlist_peek(TokenList *t)
{
    if (t->idx < t->sz)
    {
        return t->arr[t->idx];
    }

    Token eof = { T_EOF, NULL, 0, 0 };
    return eof;
}

static Token tlist_next(TokenList *t)
{
    if (t->idx < t->sz)
    {
        return t->arr[t->idx++];
    }

    Token eof = { T_EOF, NULL, 0, 0 };
    return eof;
}

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

static void rt_init(RtMap *m, int cap)
{
    m->sz = 0;
    m->cap = cap;
    m->arr = malloc(sizeof(RtEntry) * m->cap);
}

static void rt_set(RtMap *m, int id, double v)
{
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

static double rt_get(RtMap *m, int id)
{
    for (int i = 0; i < m->sz; i++)
    {
        if (m->arr[i].id == id)
        {
            return m->arr[i].val;
        }
    }

    return 0.0;
}

static void rt_free(RtMap *m)
{
    free(m->arr);
}

// AST node types
typedef enum {
    N_NUMBER,
    N_HASH,
    N_UNARY,
    N_BINARY,
    N_FUNC,
    N_ASSIGN
} NodeType;

typedef enum {
    U_NEG,
    U_NOT,
    U_BITNOT
} UnaryOp;

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

typedef struct Node {
    NodeType type;
    int pos; // position in input for errors
    union {
        double number;
        int hashId;
        struct {
            UnaryOp op;
            struct Node *child;
        } unary;
        struct {
            BinaryOp op;
            struct Node *left;
            struct Node *right;
        } binary;
        struct {
            char *name;
            struct Node *arg;
        } func;
        struct {
            int id;
            struct Node *rhs;
        } assign;
    } v;
} Node;

static Node* node_number(double val, int pos)
{
    Node *n = malloc(sizeof(Node));
    n->type = N_NUMBER;
    n->pos = pos;
    n->v.number = val;
    return n;
}

static Node* node_hash(int id, int pos)
{
    Node *n = malloc(sizeof(Node));
    n->type = N_HASH;
    n->pos = pos;
    n->v.hashId = id;
    return n;
}

static Node* node_unary(UnaryOp op, Node *child, int pos)
{
    Node *n = malloc(sizeof(Node));
    n->type = N_UNARY;
    n->pos = pos;
    n->v.unary.op = op;
    n->v.unary.child = child;
    return n;
}

static Node* node_binary(BinaryOp op, Node *l, Node *r, int pos)
{
    Node *n = malloc(sizeof(Node));
    n->type = N_BINARY;
    n->pos = pos;
    n->v.binary.op = op;
    n->v.binary.left = l;
    n->v.binary.right = r;
    return n;
}

static Node* node_func(const char *name, Node *arg, int pos)
{
    Node *n = malloc(sizeof(Node));
    n->type = N_FUNC;
    n->pos = pos;
    n->v.func.name = strdup(name);
    n->v.func.arg = arg;
    return n;
}

static Node* node_assign(int id, Node *rhs, int pos)
{
    Node *n = malloc(sizeof(Node));
    n->type = N_ASSIGN;
    n->pos = pos;
    n->v.assign.id = id;
    n->v.assign.rhs = rhs;
    return n;
}

static void free_node(Node *n)
{
    if (!n)
    {
        return;
    }

    switch (n->type)
    {
        case N_NUMBER:
            break;
        case N_HASH:
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
            free_node(n->v.func.arg);
            break;
        case N_ASSIGN:
            free_node(n->v.assign.rhs);
            break;
    }

    free(n);
}

// printing AST
static void print_node(Node *n, const char *indent, int last)
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
        case N_HASH:
            printf("#%d\n", n->v.hashId);
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
                print_node(n->v.func.arg, buf, 1);
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

// evaluation with short-circuit
static double eval_node(Node *n, RtMap *rt)
{
    if (!n)
    {
        return 0.0;
    }

    switch (n->type)
    {
        case N_NUMBER:
            return n->v.number;
        case N_HASH:
            return rt_get(rt, n->v.hashId);
        case N_UNARY:
        {
            double v = eval_node(n->v.unary.child, rt);
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
                    return eval_node(n->v.binary.left, rt) + eval_node(n->v.binary.right, rt);
                case B_SUB:
                    return eval_node(n->v.binary.left, rt) - eval_node(n->v.binary.right, rt);
                case B_MUL:
                    return eval_node(n->v.binary.left, rt) * eval_node(n->v.binary.right, rt);
                case B_DIV:
                {
                    double r = eval_node(n->v.binary.right, rt);
                    if (r == 0)
                    {
                        fprintf(stderr, "Runtime error: division by zero at pos %d\n", n->pos);
                        exit(1);
                    }

                    return eval_node(n->v.binary.left, rt) / r;
                }
                case B_LSHIFT:
                    return (double) (((long) eval_node(n->v.binary.left, rt)) << (int) eval_node(n->v.binary.right, rt));
                case B_RSHIFT:
                    return (double) (((long) eval_node(n->v.binary.left, rt)) >> (int) eval_node(n->v.binary.right, rt));
                case B_GT:
                    return eval_node(n->v.binary.left, rt) > eval_node(n->v.binary.right, rt) ? 1.0 : 0.0;
                case B_GTE:
                    return eval_node(n->v.binary.left, rt) >= eval_node(n->v.binary.right, rt) ? 1.0 : 0.0;
                case B_LT:
                    return eval_node(n->v.binary.left, rt) < eval_node(n->v.binary.right, rt) ? 1.0 : 0.0;
                case B_LTE:
                    return eval_node(n->v.binary.left, rt) <= eval_node(n->v.binary.right, rt) ? 1.0 : 0.0;
                case B_EQ:
                    return eval_node(n->v.binary.left, rt) == eval_node(n->v.binary.right, rt) ? 1.0 : 0.0;
                case B_NEQ:
                    return eval_node(n->v.binary.left, rt) != eval_node(n->v.binary.right, rt) ? 1.0 : 0.0;
                case B_BITAND:
                    return (double) (((long) eval_node(n->v.binary.left, rt)) & ((long) eval_node(n->v.binary.right, rt)));
                case B_BITXOR:
                    return (double) (((long) eval_node(n->v.binary.left, rt)) ^ ((long) eval_node(n->v.binary.right, rt)));
                case B_BITOR:
                    return (double) (((long) eval_node(n->v.binary.left, rt)) | ((long) eval_node(n->v.binary.right, rt)));
                case B_ANDAND:
                {
                    double lv = eval_node(n->v.binary.left, rt);
                    if (lv == 0.0)
                        return 0.0;
                    double rv = eval_node(n->v.binary.right, rt);
                    return rv != 0.0 ? 1.0 : 0.0;
                }
                case B_OROR:
                {
                    double lv = eval_node(n->v.binary.left, rt);
                    if (lv != 0.0)
                        return 1.0;
                    double rv = eval_node(n->v.binary.right, rt);
                    return rv != 0.0 ? 1.0 : 0.0;
                }
            }
            break;
        }
        case N_FUNC:
        {
            double a = eval_node(n->v.func.arg, rt);
            if (strcmp(n->v.func.name, "sin") == 0)
                return sin(a);
            if (strcmp(n->v.func.name, "cos") == 0)
                return cos(a);
            if (strcmp(n->v.func.name, "exp") == 0)
                return exp(a);
            fprintf(stderr, "Runtime error: unknown function %s at pos %d\n", n->v.func.name, n->pos);
            exit(1);
        }
        case N_ASSIGN:
        {
            double v = eval_node(n->v.assign.rhs, rt);
            rt_set(rt, n->v.assign.id, v);
            return v;
        }
    }

    return 0.0;
}

// Parser functions follow grammar and precedence
static Node* parse_assign(TokenList *toks);
static Node* parse_logical_or_node(TokenList *toks);
static Node* parse_logical_and_node(TokenList *toks);
static Node* parse_bitor_node(TokenList *toks);
static Node* parse_bitxor_node(TokenList *toks);
static Node* parse_bitand_node(TokenList *toks);
static Node* parse_equality_node(TokenList *toks);
static Node* parse_relational_node(TokenList *toks);
static Node* parse_shift_node(TokenList *toks);
static Node* parse_add_node(TokenList *toks);
static Node* parse_multiply_node(TokenList *toks);
static Node* parse_unary_node(TokenList *toks);
static Node* parse_power_node(TokenList *toks);
static Node* parse_primary_node(TokenList *toks);

static int match(TokenList *t, TokenType ty)
{
    if (tlist_peek(t).type == ty)
    {
        tlist_next(t);
        return 1;
    }

    return 0;
}

static Node* parse_assign(TokenList *toks)
{
    Token cur = tlist_peek(toks);
    if (cur.type == T_HASH && toks->idx + 1 < toks->sz && toks->arr[toks->idx + 1].type == T_ASSIGN)
    {
        Token h = tlist_next(toks); // consume HASH
        Token a = tlist_next(toks); // consume ASSIGN
        Node *rhs = parse_assign(toks); // right-assoc
        int id = atoi(h.text);
        return node_assign(id, rhs, a.pos);
    }

    return parse_logical_or_node(toks);
}

static Node* parse_logical_or_node(TokenList *toks)
{
    Node *left = parse_logical_and_node(toks);
    while (match(toks, T_OROR))
    {
        Node *right = parse_logical_and_node(toks);
        left = node_binary(B_OROR, left, right, left->pos);
    }

    return left;
}

static Node* parse_logical_and_node(TokenList *toks)
{
    Node *left = parse_bitor_node(toks);
    while (match(toks, T_ANDAND))
    {
        Node *right = parse_bitor_node(toks);
        left = node_binary(B_ANDAND, left, right, left->pos);
    }

    return left;
}

static Node* parse_bitor_node(TokenList *toks)
{
    Node *left = parse_bitxor_node(toks);
    while (match(toks, T_PIPE))
    {
        Node *r = parse_bitxor_node(toks);
        left = node_binary(B_BITOR, left, r, left->pos);
    }

    return left;
}

static Node* parse_bitxor_node(TokenList *toks)
{
    Node *left = parse_bitand_node(toks);
    while (match(toks, T_CARET))
    {
        Node *r = parse_bitand_node(toks);
        left = node_binary(B_BITXOR, left, r, left->pos);
    }

    return left;
}

static Node* parse_bitand_node(TokenList *toks)
{
    Node *left = parse_equality_node(toks);
    while (match(toks, T_AMP))
    {
        Node *r = parse_equality_node(toks);
        left = node_binary(B_BITAND, left, r, left->pos);
    }

    return left;
}

static Node* parse_equality_node(TokenList *toks)
{
    Node *left = parse_relational_node(toks);
    while (1)
    {
        if (match(toks, T_EQ))
        {
            Node *r = parse_relational_node(toks);
            left = node_binary(B_EQ, left, r, left->pos);
        }
        else if (match(toks, T_NEQ))
        {
            Node *r = parse_relational_node(toks);
            left = node_binary(B_NEQ, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

static Node* parse_relational_node(TokenList *toks)
{
    Node *left = parse_shift_node(toks);
    while (1)
    {
        if (match(toks, T_GT))
        {
            Node *r = parse_shift_node(toks);
            left = node_binary(B_GT, left, r, left->pos);
        }
        else if (match(toks, T_GTE))
        {
            Node *r = parse_shift_node(toks);
            left = node_binary(B_GTE, left, r, left->pos);
        }
        else if (match(toks, T_LT))
        {
            Node *r = parse_shift_node(toks);
            left = node_binary(B_LT, left, r, left->pos);
        }
        else if (match(toks, T_LTE))
        {
            Node *r = parse_shift_node(toks);
            left = node_binary(B_LTE, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

static Node* parse_shift_node(TokenList *toks)
{
    Node *left = parse_add_node(toks);
    while (1)
    {
        if (match(toks, T_LSHIFT))
        {
            Node *r = parse_add_node(toks);
            left = node_binary(B_LSHIFT, left, r, left->pos);
        }
        else if (match(toks, T_RSHIFT))
        {
            Node *r = parse_add_node(toks);
            left = node_binary(B_RSHIFT, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

static Node* parse_add_node(TokenList *toks)
{
    Node *left = parse_multiply_node(toks);
    while (1)
    {
        if (match(toks, T_PLUS))
        {
            Node *r = parse_multiply_node(toks);
            left = node_binary(B_ADD, left, r, left->pos);
        }
        else if (match(toks, T_MINUS))
        {
            Node *r = parse_multiply_node(toks);
            left = node_binary(B_SUB, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

static Node* parse_multiply_node(TokenList *toks)
{
    Node *left = parse_unary_node(toks);
    while (1)
    {
        if (match(toks, T_MUL))
        {
            Node *r = parse_unary_node(toks);
            left = node_binary(B_MUL, left, r, left->pos);
        }
        else if (match(toks, T_DIV))
        {
            Node *r = parse_unary_node(toks);
            left = node_binary(B_DIV, left, r, left->pos);
        }
        else
            break;
    }

    return left;
}

static Node* parse_unary_node(TokenList *toks)
{
    if (match(toks, T_NOT))
    {
        Node *op = parse_unary_node(toks);
        return node_unary(U_NOT, op, op->pos);
    }

    if (match(toks, T_TILDE))
    {
        Node *op = parse_unary_node(toks);
        return node_unary(U_BITNOT, op, op->pos);
    }

    if (match(toks, T_MINUS))
    {
        Node *op = parse_unary_node(toks);
        return node_unary(U_NEG, op, op->pos);
    }

    return parse_power_node(toks);
}

static Node* parse_power_node(TokenList *toks)
{
    Token cur = tlist_peek(toks);
    if (cur.type == T_IDENT && strcmp(cur.text, "exp") == 0)
    {
        tlist_next(toks);
        if (!match(toks, T_LP))
        {
            fprintf(stderr, "Syntax error: expected '(' after exp at %d\n", cur.pos);
            exit(1);
        }
        Node *arg = parse_assign(toks);
        if (!match(toks, T_RP))
        {
            fprintf(stderr, "Syntax error: expected ')' after exp at %d\n", cur.pos);
            exit(1);
        }
        return node_func("exp", arg, cur.pos);
    }

    if (cur.type == T_IDENT && (strcmp(cur.text, "sin") == 0 || strcmp(cur.text, "cos") == 0))
    {
        tlist_next(toks);
        if (!match(toks, T_LP))
        {
            fprintf(stderr, "Syntax error: expected '(' after %s at %d\n", cur.text, cur.pos);
            exit(1);
        }
        Node *arg = parse_assign(toks);
        if (!match(toks, T_RP))
        {
            fprintf(stderr, "Syntax error: expected ')' after %s at %d\n", cur.text, cur.pos);
            exit(1);
        }
        return node_func(cur.text, arg, cur.pos);
    }

    return parse_primary_node(toks);
}

static Node* parse_primary_node(TokenList *toks)
{
    Token t = tlist_peek(toks);
    if (t.type == T_NUM)
    {
        Token tk = tlist_next(toks);
        return node_number(tk.num, tk.pos);
    }

    if (t.type == T_HASH)
    {
        Token tk = tlist_next(toks);
        int id = atoi(tk.text);
        return node_hash(id, tk.pos);
    }

    if (t.type == T_IDENT)
    {
        fprintf(stderr, "Syntax error: unexpected identifier '%s' at %d\n", t.text, t.pos);
        exit(1);
    }

    if (match(toks, T_LP))
    {
        Node *v = parse_assign(toks);
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

// Tokenizer
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
            Token t = { T_EOF, NULL, 0 };
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

        // numbers
        if (isdigit((unsigned char)c) || c == '.')
        {
            int start = i;
            while (i < n && (isdigit((unsigned char)s[i]) || s[i] == '.'))
                i++;
            int len = i - start;
            char *txt = strndup(s + start, len);
            double v = strtod(txt, NULL);
            Token t = { T_NUM, txt, v };
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
                Token t = { T_INVALID, NULL, 0 };
                tlist_push(out, t);
                break;
            }
            int len = i - start;
            char *txt = strndup(s + start, len);
            Token t = { T_HASH, txt, 0 };
            tlist_push(out, t);
            continue;
        }

        if (isalpha((unsigned char )c))
        {
            int start = i;
            while (i < n && isalpha((unsigned char )s[i]))
                i++;
            int len = i - start;
            char *txt = strndup(s + start, len);
            Token t = { T_IDENT, txt, 0 };
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
            default:
                {
                Token t = { T_INVALID, NULL, 0 };
                tlist_push(out, t);
                i++;
                break;
            }
        }
    }
}

// small helper to show error with caret
static void print_error_with_caret(const char *line, int pos)
{
    fprintf(stderr, "%s\n", line);
    for (int i = 0; i < pos && line[i]; i++)
    {
        fputc(line[i] == '\t' ? '\t' : ' ', stderr);
    }

    fprintf(stderr, "^\n");
}

// Optimization: constant-fold subtrees that do not contain any realtime hash (#id)
// Realtime hash nodes (N_HASH) must not be folded because their values may change concurrently.

// Return 1 if subtree contains a hash node
static int node_contains_hash(Node *n)
{
    if (!n)
    {
        return 0;
    }

    switch (n->type)
    {
        case N_HASH:
            return 1;
        case N_NUMBER:
            return 0;
        case N_UNARY:
            return node_contains_hash(n->v.unary.child);
        case N_BINARY:
            return node_contains_hash(n->v.binary.left) || node_contains_hash(n->v.binary.right);
        case N_FUNC:
            return node_contains_hash(n->v.func.arg);
        case N_ASSIGN:
            return node_contains_hash(n->v.assign.rhs); // assignment lhs is id, not a hash node
        default:
            return 0;
    }
}

// Helper to get number from a node (assumes node->type == N_NUMBER)
static double node_get_number(Node *n)
{
    return n->v.number;
}

// Constant-folding optimizer; returns possibly new node (caller must use returned pointer).
static Node* optimize_node(Node *n)
{
    if (!n)
    {
        return NULL;
    }

    switch (n->type)
    {
        case N_NUMBER:
            case N_HASH:
            return n;

        case N_UNARY:
        {
            n->v.unary.child = optimize_node(n->v.unary.child);
            if (!node_contains_hash(n) && n->v.unary.child && n->v.unary.child->type == N_NUMBER)
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
            if (!node_contains_hash(n) && n->v.binary.left && n->v.binary.right
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
            n->v.func.arg = optimize_node(n->v.func.arg);
            if (!node_contains_hash(n) && n->v.func.arg && n->v.func.arg->type == N_NUMBER)
            {
                double a = node_get_number(n->v.func.arg);
                double res;
                if (strcmp(n->v.func.name, "sin") == 0)
                    res = sin(a);
                else if (strcmp(n->v.func.name, "cos") == 0)
                    res = cos(a);
                else if (strcmp(n->v.func.name, "exp") == 0)
                    res = exp(a);
                else
                    return n; // unknown func, don't fold
                int pos = n->pos;
                free_node(n);
                return node_number(res, pos);
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

// Top-level optimizer wrapper
static Node* optimize_ast(Node *root)
{
    return optimize_node(root);
}

void eval_main(void)
{
    char line[8192];
    RtMap rt = { 0 };

    rt_init(&rt, 8192);
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
        Node *ast = NULL;

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
        ast = optimize_ast(ast);
        printf("Optimized AST:\n");
        print_node(ast, "", 1);

        // evaluate
        double res = eval_node(ast, &rt);
        printf("Result: %g\n", res);
        free_node(ast);
        tlist_free(&toks);
        printf("expr> ");
    }

    rt_free(&rt);
}
