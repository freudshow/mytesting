/**********************************************************************************
 * eval.c : A simple expression calculator
 * run: gcc eval.c -lm -o eval
 * --------------------------------------------------------------------------------
 * A simple expression calculator supporting:
 * grammar of ExprCalc:
 *
 * prog      : stmt EOF ;
 *
 * stmt      : assignStmt
 *           | expr
 *           ;
 *
 * assignStmt: HASH ASSIGN expr ;
 *
 * expr      : logicalOr ;
 *
 * logicalOr : logicalAnd ( OROR logicalAnd )* ;
 * logicalAnd: bitOr ( ANDAND bitOr )* ;
 * bitOr     : bitXor ( PIPE bitXor )* ;
 * bitXor    : bitAnd ( CARET bitAnd )* ;
 * bitAnd    : equality ( AMP equality )* ;
 * equality  : relational ( EQ relational )* ;
 * relational: shift ( (GT | GTE | LT | LTE) shift )* ;
 * shift     : add ( (LSHIFT | RSHIFT) add )* ;
 * add       : mul ( (PLUS | MINUS) mul )* ;
 * mul       : unary ( (MULT | DIV) unary )* ;
 * unary     : ( NOT | TILDE | MINUS ) unary
 *           | primary
 *           ;
 * primary   : NUMBER
 *           | HASH
 *           | LPAREN expr RPAREN
 *           | functionCall
 *           ;
 *
 * functionCall
 *           : IDENT LPAREN expr RPAREN
 *           ;
 *
 * // Lexer tokens (representative)
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
 * AMP     : '&' ;
 * PIPE    : '|' ;
 * CARET   : '^' ;
 * TILDE   : '~' ;
 * LSHIFT  : '<<' ;
 * RSHIFT  : '>>' ;
 * LPAREN  : '(' ;
 * RPAREN  : ')' ;
 * ASSIGN  : '=' ;
 *
 * // numbers and identifiers
 * NUMBER  : [0-9]+ ('.' [0-9]*)? | '.' [0-9]+ ;
 * HASH    : '#' [0-9]+ ;
 * IDENT   : [a-zA-Z]+ ;
 *
 * // whitespace and bad chars
 * WS      : [ \t\r\n]+ -> skip ;
 * ERROR_CHAR : . -> channel(HIDDEN) ; // or handle as error in lexer action
 ***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <setjmp.h>

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
    T_EOF,
    T_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char *text; // for HASH (id) or IDENT or number text
    double num;
} Token;

const char *input_s;
int input_pos, input_len;

void skipws()
{
    while (input_pos < input_len && isspace((unsigned char )input_s[input_pos]))
        input_pos++;
}

Token make_tok(TokenType t, const char *txt)
{
    Token tk;
    tk.type = t;
    tk.text = txt ? strdup(txt) : NULL;
    tk.num = 0.0;
    return tk;
}

Token make_num(double v, const char *txt)
{
    Token tk = make_tok(T_NUM, txt);
    tk.num = v;
    return tk;
}

static Token next_token()
{
    skipws();
    if (input_pos >= input_len)
        return make_tok(T_EOF, NULL);
    char c = input_s[input_pos];

    // multi-char ops
    if (c == '&' && input_pos + 1 < input_len && input_s[input_pos + 1] == '&')
    {
        input_pos += 2;
        return make_tok(T_ANDAND, "&&");
    }
    if (c == '|' && input_pos + 1 < input_len && input_s[input_pos + 1] == '|')
    {
        input_pos += 2;
        return make_tok(T_OROR, "||");
    }
    if (c == '<' && input_pos + 1 < input_len && input_s[input_pos + 1] == '<')
    {
        input_pos += 2;
        return make_tok(T_LSHIFT, "<<");
    }
    if (c == '>' && input_pos + 1 < input_len && input_s[input_pos + 1] == '>')
    {
        input_pos += 2;
        return make_tok(T_RSHIFT, ">>");
    }
    if (c == '>' && input_pos + 1 < input_len && input_s[input_pos + 1] == '=')
    {
        input_pos += 2;
        return make_tok(T_GTE, ">=");
    }
    if (c == '<' && input_pos + 1 < input_len && input_s[input_pos + 1] == '=')
    {
        input_pos += 2;
        return make_tok(T_LTE, "<=");
    }
    if (c == '=' && input_pos + 1 < input_len && input_s[input_pos + 1] == '=')
    {
        input_pos += 2;
        return make_tok(T_EQ, "==");
    }

    // single
    if (c == '+')
    {
        input_pos++;
        return make_tok(T_PLUS, "+");
    }
    if (c == '-')
    {
        input_pos++;
        return make_tok(T_MINUS, "-");
    }
    if (c == '*')
    {
        input_pos++;
        return make_tok(T_MUL, "*");
    }
    if (c == '/')
    {
        input_pos++;
        return make_tok(T_DIV, "/");
    }
    if (c == '(')
    {
        input_pos++;
        return make_tok(T_LP, "(");
    }
    if (c == ')')
    {
        input_pos++;
        return make_tok(T_RP, ")");
    }
    if (c == '!')
    {
        input_pos++;
        return make_tok(T_NOT, "!");
    }
    if (c == '>')
    {
        input_pos++;
        return make_tok(T_GT, ">");
    }
    if (c == '<')
    {
        input_pos++;
        return make_tok(T_LT, "<");
    }
    if (c == '&')
    {
        input_pos++;
        return make_tok(T_AMP, "&");
    }
    if (c == '|')
    {
        input_pos++;
        return make_tok(T_PIPE, "|");
    }
    if (c == '^')
    {
        input_pos++;
        return make_tok(T_CARET, "^");
    }
    if (c == '~')
    {
        input_pos++;
        return make_tok(T_TILDE, "~");
    }
    if (c == '=')
    {
        input_pos++;
        return make_tok(T_ASSIGN, "=");
    }

    if (c == '#')
    {
        input_pos++;
        int start = input_pos;
        if (input_pos < input_len && isdigit((unsigned char )input_s[input_pos]))
        {
            while (input_pos < input_len && isdigit((unsigned char )input_s[input_pos]))
                input_pos++;
            int n = input_pos - start;
            char *txt = strndup(input_s + start, n);
            Token t = make_tok(T_HASH, txt);
            free(txt); // token keeps copy created by make_tok
            return t;
        }
        else
        {
            input_pos++;
            return make_tok(T_INVALID, NULL);
        }
    }

    if (isdigit((unsigned char)c) || c == '.')
    {
        char *endptr;
        double v = strtod(input_s + input_pos, &endptr);
        int consumed = (int) (endptr - (input_s + input_pos));
        if (consumed == 0)
        {
            input_pos++;
            return make_tok(T_INVALID, NULL);
        }
        char *txt = strndup(input_s + input_pos, consumed);
        input_pos += consumed;
        Token t = make_num(v, txt);
        free(txt);
        return t;
    }

    if (isalpha((unsigned char )c))
    {
        int start = input_pos;
        while (input_pos < input_len && isalpha((unsigned char )input_s[input_pos]))
            input_pos++;
        int n = input_pos - start;
        char *id = strndup(input_s + start, n);
        Token t = make_tok(T_IDENT, id);
        free(id);
        return t;
    }

    // unknown char
    input_pos++;
    return make_tok(T_INVALID, NULL);
}

// simple token vector
typedef struct {
    Token *arr;
    int sz, cap;
    int idx;
} TokVec;

void vec_init(TokVec *v)
{
    v->sz = 0;
    v->cap = 16;
    v->arr = malloc(sizeof(Token) * v->cap);
    v->idx = 0;
}

void vec_push(TokVec *v, Token t)
{
    if (v->sz == v->cap)
    {
        v->cap *= 2;
        v->arr = realloc(v->arr, sizeof(Token) * v->cap);
    }
    v->arr[v->sz++] = t;
}

Token vec_peek(TokVec *v)
{
    if (v->idx < v->sz)
        return v->arr[v->idx];
    return make_tok(T_EOF, NULL);
}

Token vec_next(TokVec *v)
{
    if (v->idx < v->sz)
        return v->arr[v->idx++];
    return make_tok(T_EOF, NULL);
}

void vec_free(TokVec *v)
{
    for (int i = 0; i < v->sz; i++)
        if (v->arr[i].text)
            free(v->arr[i].text);
    free(v->arr);
}

// RT mapping
typedef struct {
    int id;
    double val;
} RtEntry;

typedef struct {
    RtEntry *arr;
    int sz, cap;
} RtMap;

void rt_init(RtMap *m)
{
    m->sz = 0;
    m->cap = 16;
    m->arr = malloc(sizeof(RtEntry) * m->cap);
}

void rt_set(RtMap *m, int id, double v)
{
    for (int i = 0; i < m->sz; i++)
        if (m->arr[i].id == id)
        {
            m->arr[i].val = v;
            return;
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

double rt_get(RtMap *m, int id)
{
    for (int i = 0; i < m->sz; i++)
        if (m->arr[i].id == id)
            return m->arr[i].val;
    return 0.0;
}

void rt_free(RtMap *m)
{
    free(m->arr);
}

// Parser (recursive descent) prototypes
double parse_logical_or(TokVec *v, RtMap *rt);
double parse_logical_and(TokVec *v, RtMap *rt);
double parse_bitor(TokVec *v, RtMap *rt);
double parse_bitxor(TokVec *v, RtMap *rt);
double parse_bitand(TokVec *v, RtMap *rt);
double parse_equality(TokVec *v, RtMap *rt);
double parse_relational(TokVec *v, RtMap *rt);
double parse_shift(TokVec *v, RtMap *rt);
double parse_addsub(TokVec *v, RtMap *rt);
double parse_muldiv(TokVec *v, RtMap *rt);
double parse_unary(TokVec *v, RtMap *rt);
double parse_primary(TokVec *v, RtMap *rt);

double parse_primary(TokVec *v, RtMap *rt)
{
    Token t = vec_peek(v);
    if (t.type == T_NUM)
    {
        vec_next(v);
        return t.num;
    }

    if (t.type == T_HASH)
    {
        vec_next(v);
        int id = atoi(t.text);
        return rt_get(rt, id);
    }

    if (t.type == T_IDENT)
    {
        vec_next(v);
        if (strcmp(t.text, "sin") == 0 || strcmp(t.text, "cos") == 0 || strcmp(t.text, "exp") == 0)
        {
            Token p = vec_peek(v);
            if (p.type != T_LP)
            {
                fprintf(stderr, "Syntax error: expected ( after %s\n", t.text);
                longjmp(*(jmp_buf*) NULL, 1);
            }
            vec_next(v);
            double a = parse_logical_or(v, rt);
            Token rp = vec_peek(v);
            if (rp.type != T_RP)
            {
                fprintf(stderr, "Syntax error: expected )\n");
                longjmp(*(jmp_buf*) NULL, 1);
            }
            vec_next(v);
            if (strcmp(t.text, "sin") == 0)
                return sin(a);
            if (strcmp(t.text, "cos") == 0)
                return cos(a);
            return exp(a);
        }
        else
        {
            fprintf(stderr, "Unknown function: %s\n", t.text);
            longjmp(*(jmp_buf*) NULL, 1);
        }
    }

    if (t.type == T_LP)
    {
        vec_next(v);
        double vret = parse_logical_or(v, rt);
        Token rp = vec_peek(v);
        if (rp.type != T_RP)
        {
            fprintf(stderr, "Syntax error: expected )\n");
            longjmp(*(jmp_buf*) NULL, 1);
        }
        vec_next(v);
        return vret;
    }

    fprintf(stderr, "Unexpected token in primary\n");
    longjmp(*(jmp_buf*) NULL, 1);
    return 0.0;
}

double parse_unary(TokVec *v, RtMap *rt)
{
    Token t = vec_peek(v);
    if (t.type == T_NOT)
    {
        vec_next(v);
        double r = parse_unary(v, rt);
        return r != 0.0 ? 0.0 : 1.0;
    }

    if (t.type == T_TILDE)
    {
        vec_next(v);
        double r = parse_unary(v, rt);
        int64_t iv = (int64_t) r;
        return (double) (~iv);
    }

    if (t.type == T_MINUS)
    {
        vec_next(v);
        double r = parse_unary(v, rt);
        return -r;
    }

    return parse_primary(v, rt);
}

double parse_muldiv(TokVec *v, RtMap *rt)
{
    double left = parse_unary(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_MUL)
        {
            vec_next(v);
            double r = parse_unary(v, rt);
            left *= r;
        }
        else if (t.type == T_DIV)
        {
            vec_next(v);
            double r = parse_unary(v, rt);
            if (r == 0.0)
            {
                fprintf(stderr, "Division by zero\n");
                longjmp(*(jmp_buf*) NULL, 1);
            }
            left /= r;
        }
        else
            break;
    }
    return left;
}

double parse_addsub(TokVec *v, RtMap *rt)
{
    double left = parse_muldiv(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_PLUS)
        {
            vec_next(v);
            double r = parse_muldiv(v, rt);
            left += r;
        }
        else if (t.type == T_MINUS)
        {
            vec_next(v);
            double r = parse_muldiv(v, rt);
            left -= r;
        }
        else
            break;
    }

    return left;
}

double parse_shift(TokVec *v, RtMap *rt)
{
    double left = parse_addsub(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_LSHIFT)
        {
            vec_next(v);
            double r = parse_addsub(v, rt);
            int64_t lv = (int64_t) left, rv = (int64_t) r;
            left = (double) (lv << rv);
        }
        else if (t.type == T_RSHIFT)
        {
            vec_next(v);
            double r = parse_addsub(v, rt);
            int64_t lv = (int64_t) left, rv = (int64_t) r;
            left = (double) (lv >> rv);
        }
        else
        {
            break;
        }
    }

    return left;
}

double parse_relational(TokVec *v, RtMap *rt)
{
    double left = parse_shift(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_GT)
        {
            vec_next(v);
            double r = parse_shift(v, rt);
            left = left > r ? 1.0 : 0.0;
        }
        else if (t.type == T_GTE)
        {
            vec_next(v);
            double r = parse_shift(v, rt);
            left = left >= r ? 1.0 : 0.0;
        }
        else if (t.type == T_LT)
        {
            vec_next(v);
            double r = parse_shift(v, rt);
            left = left < r ? 1.0 : 0.0;
        }
        else if (t.type == T_LTE)
        {
            vec_next(v);
            double r = parse_shift(v, rt);
            left = left <= r ? 1.0 : 0.0;
        }
        else
            break;
    }
    return left;
}

double parse_equality(TokVec *v, RtMap *rt)
{
    double left = parse_relational(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_EQ)
        {
            vec_next(v);
            double r = parse_relational(v, rt);
            left = left == r ? 1.0 : 0.0;
        }
        else
            break;
    }
    return left;
}

double parse_bitand(TokVec *v, RtMap *rt)
{
    double left = parse_equality(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_AMP)
        {
            vec_next(v);
            double r = parse_equality(v, rt);
            int64_t lv = (int64_t) left, rv = (int64_t) r;
            left = (double) (lv & rv);
        }
        else
            break;
    }
    return left;
}
double parse_bitxor(TokVec *v, RtMap *rt)
{
    double left = parse_bitand(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_CARET)
        {
            vec_next(v);
            double r = parse_bitand(v, rt);
            int64_t lv = (int64_t) left, rv = (int64_t) r;
            left = (double) (lv ^ rv);
        }
        else
            break;
    }
    return left;
}
double parse_bitor(TokVec *v, RtMap *rt)
{
    double left = parse_bitxor(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_PIPE)
        {
            vec_next(v);
            double r = parse_bitxor(v, rt);
            int64_t lv = (int64_t) left, rv = (int64_t) r;
            left = (double) (lv | rv);
        }
        else
            break;
    }
    return left;
}

double parse_logical_and(TokVec *v, RtMap *rt)
{
    double left = parse_bitor(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_ANDAND)
        {
            vec_next(v);
            if (left == 0.0)
            { // short-circuit: still consume RHS syntactically
                parse_bitor(v, rt);
                left = 0.0;
            }
            else
            {
                double r = parse_bitor(v, rt);
                left = (r != 0.0) ? 1.0 : 0.0;
            }
        }
        else
            break;
    }
    return left;
}

double parse_logical_or(TokVec *v, RtMap *rt)
{
    double left = parse_logical_and(v, rt);
    while (1)
    {
        Token t = vec_peek(v);
        if (t.type == T_OROR)
        {
            vec_next(v);
            if (left != 0.0)
            {
                parse_logical_and(v, rt);
                left = 1.0;
            }
            else
            {
                double r = parse_logical_and(v, rt);
                left = (r != 0.0) ? 1.0 : 0.0;
            }
        }
        else
            break;
    }
    return left;
}

void eval_main(void)
{
    char line[4096];
    RtMap rt;
    rt_init(&rt);

    printf("expr> ");
    while (fgets(line, sizeof(line), stdin))
    {
        if (line[0] == '\n' || line[0] == 0)
        {
            break;
        }

        input_s = line;
        input_pos = 0;
        input_len = (int) strlen(line);
        TokVec tokens;
        vec_init(&tokens);

        // tokenize
        while (1)
        {
            Token t = next_token();
            vec_push(&tokens, t);
            if (t.type == T_INVALID)
            {
                fprintf(stderr, "Invalid character in input\n");
                break;
            }
            if (t.type == T_EOF)
                break;
        }
        if (tokens.sz > 0 && tokens.arr[tokens.sz - 1].type == T_INVALID)
        {
            vec_free(&tokens);
            printf("expr> ");
            continue;
        }
        tokens.idx = 0;

        // statement: assignment (#id = expr) or expression
        Token first = vec_peek(&tokens);
        if (first.type == T_HASH)
        {
            // lookahead
            if (tokens.sz >= 2 && tokens.arr[1].type == T_ASSIGN)
            {
                Token hash = vec_next(&tokens); // consume
                vec_next(&tokens); // consume =
                // parse rhs
                double value = 0.0;
                // use setjmp/longjmp? keep simple: parse and check errors by try-like behavior with prints
                value = parse_logical_or(&tokens, &rt);
                Token after = vec_peek(&tokens);
                if (after.type != T_EOF)
                {
                    fprintf(stderr, "Syntax error: unexpected token after assignment\n");
                }
                else
                {
                    int id = atoi(hash.text);
                    rt_set(&rt, id, value);
                    printf("Assigned #%d = %g\n", id, value);
                }
                vec_free(&tokens);
                printf("expr> ");
                continue;
            }
        }
        // else expression
        double result = 0.0;
        result = parse_logical_or(&tokens, &rt);
        Token after = vec_peek(&tokens);
        if (after.type != T_EOF)
        {
            fprintf(stderr, "Syntax error: unexpected token\n");
        }
        else
        {
            printf("%g\n", result);
        }

        vec_free(&tokens);
        printf("expr> ");
    }

    rt_free(&rt);
}
