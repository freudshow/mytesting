#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TOKEN_STRING_MAX_SIZE  128

// Token types
typedef enum {
    TOKEN_START,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_BIT_OR,
    TOKEN_BIT_AND,
    TOKEN_BIT_XOR,
    TOKEN_LEFT_SHIFT,
    TOKEN_RIGHT_SHIFT,
    TOKEN_SIN,
    TOKEN_COS,
    TOKEN_EXPONENTIAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_REALDB,
    TOKEN_END
} TokenType;

// Token struct
typedef struct {
    TokenType type;
    char str[TOKEN_STRING_MAX_SIZE];
    unsigned int pos;
    union {
        double numValue;
        int intValue;
    } value;
} Token;

#define EMPTY_TO_S              ( -1 )  //栈的初始指针索引
#define MIN_STACK_ARRAY_SIZE    ( 32 )  //栈的最小容量

typedef Token elementType;

struct stackArray;
typedef struct stackArray stackArray_s;
typedef stackArray_s *pStackArray;

typedef int (*pIsEmptyArray_f)(pStackArray s);
typedef int (*pIsFullArray_f)(pStackArray s);
typedef void (*pDisposeStackArray_f)(pStackArray s);
typedef void (*pMakeEmptyArray_f)(pStackArray s);
typedef void (*pPushArray_f)(elementType e, pStackArray s);
typedef elementType (*pTopArray_f)(pStackArray s);
typedef elementType (*pPopArray_f)(pStackArray s);

struct stackArray {
    int capacity;
    int topIdx;
    elementType *array;

    pIsEmptyArray_f isEmpty;
    pIsFullArray_f isFull;
    pDisposeStackArray_f dispose;
    pMakeEmptyArray_f makeEmpty;
    pPushArray_f push;
    pTopArray_f top;
    pPopArray_f pop;
};

u32 tokenizer(const char *input, Token *tokens)
{
    u32 tokenCount = 0;
    Token *pToken = tokens;
    u32 inputlen = strlen(input);

    u32 position = 0;
    while (input[position] != '\0')
    {
        if (input[position] == ' ' || input[position] == '\t' || input[position] == '\r' || input[position] == '\n')
        {
            position++;
            continue;
        }

        if (input[position] == '#')         //read real database number
        {
            if (position >= inputlen || input[position + 1] < '0' || input[position + 1] > '9')
            {
                printf("Invalid character: %c, position: %u\n", input[position], position);
                exit(1);
            }

            pToken->pos = position;
            position++;

            int i = 0;
            while (input[position] >= '0' && input[position] <= '9')
            {
                pToken->str[i] = input[position];
                position++;
                i++;
            }

            pToken->str[i] = '\0';
            pToken->type = TOKEN_REALDB;
            pToken->value.intValue = atol(pToken->str);

            tokenCount++;
            pToken++;
        }
        else if (input[position] >= '0' && input[position] <= '9') //read numbers, including integer and float
        {
            pToken->pos = position;

            int i = 0;

            while (input[position] >= '0' && input[position] <= '9')
            {
                pToken->str[i] = input[position];
                position++;
                i++;
            }

            if (input[position] == '.')
            {
                pToken->str[i] = input[position];
                position++;
                i++;

                while (input[position] >= '0' && input[position] <= '9')
                {
                    pToken->str[i] = input[position];
                    position++;
                    i++;
                }

                pToken->str[i] = '\0';
                pToken->type = TOKEN_FLOAT;
                pToken->value.numValue = atof(pToken->str);
            }
            else
            {
                pToken->str[i] = '\0';
                pToken->type = TOKEN_INTEGER;
                pToken->value.intValue = atol(pToken->str);
            }

            tokenCount++;
            pToken++;
        }
        else        //read operators
        {
            switch (input[position])
            {
                case '+':
                    pToken->type = TOKEN_PLUS;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '-':
                    pToken->type = TOKEN_MINUS;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '*':
                    pToken->type = TOKEN_MULTIPLY;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '/':
                    pToken->type = TOKEN_DIVIDE;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '|':
                    pToken->type = TOKEN_BIT_OR;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '&':
                    pToken->type = TOKEN_BIT_AND;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '^':
                    pToken->type = TOKEN_BIT_XOR;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case '<':
                    if (position < inputlen - 1 && input[position + 1] == '<')
                    {
                        pToken->type = TOKEN_LEFT_SHIFT;
                        pToken->pos = position;
                        strncpy(pToken->str, "<<", sizeof(pToken->str) - 1);
                        position++;
                    }
                    else
                    {
                        printf("Invalid character: %c, position: %u\n", input[position], position);
                        exit(1);
                    }

                    break;
                case '>':
                    if (position < inputlen - 1 && input[position + 1] == '>')
                    {
                        pToken->type = TOKEN_RIGHT_SHIFT;
                        pToken->pos = position;
                        strncpy(pToken->str, ">>", sizeof(pToken->str) - 1);
                        position++;
                    }
                    else
                    {
                        printf("Invalid character: %c, position: %u\n", input[position], position);
                        exit(1);
                    }

                    break;
                case '(':
                    pToken->type = TOKEN_LPAREN;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case ')':
                    pToken->type = TOKEN_RPAREN;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';
                    break;
                case 's':
                    if (position < inputlen - 2 && input[position + 1] == 'i' && input[position + 2] == 'n')
                    {
                        pToken->type = TOKEN_SIN;
                        pToken->pos = position;
                        strncpy(pToken->str, "sin", sizeof(pToken->str) - 1);
                        position += 2;
                    }
                    else
                    {
                        printf("Invalid character: %c, position: %u\n", input[position], position);
                        exit(1);
                    }

                    break;
                case 'c':
                    if (position < inputlen - 2 && input[position + 1] == 'o' && input[position + 2] == 's')
                    {
                        pToken->type = TOKEN_COS;
                        pToken->pos = position;
                        strncpy(pToken->str, "cos", sizeof(pToken->str) - 1);
                        position += 2;
                    }
                    else
                    {
                        printf("Invalid character: %c, position: %u\n", input[position], position);
                        exit(1);
                    }

                    break;
                default:
                    printf("Invalid character: %c\n", input[position]);
                    exit(1);
            }

            position++;

            tokenCount++;
            pToken++;
        }
    }

    pToken->type = TOKEN_END;

    tokenCount++;
    pToken++;
    return tokenCount;
}

/******************************************************
 * 函数功能: 判断栈是否为空
 * ---------------------------------------------------
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 空, 返回1;
 *           非空, 返回0
 ******************************************************/
static int isEmptyArray(pStackArray s)
{
    return s->topIdx == EMPTY_TO_S;
}

/******************************************************
 * 函数功能: 判断栈是否已满
 * ---------------------------------------------------
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 已满, 返回1;
 *           不满, 返回0
 ******************************************************/
static int isFullArray(pStackArray s)
{
    return s->topIdx == s->capacity - 1;
}

/******************************************************
 * 函数功能: 使栈归零
 * ---------------------------------------------------
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
static void makeEmptyArray(pStackArray s)
{
    s->topIdx = EMPTY_TO_S;
}

/******************************************************
 * 函数功能: 释放栈占用的内存
 * ---------------------------------------------------
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
static void disposeStackArray(pStackArray s)
{
    if (s != NULL)
    {
        if (s->array != NULL)
        {
            free(s->array);
        }

        free(s);
    }
}

/******************************************************
 * 函数功能: 向栈内压入1个元素
 * ---------------------------------------------------
 * @param - e, 被压入元素
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
static void pushArray(elementType e, pStackArray s)
{
    if (isFullArray(s))
    {
        DEBUG_TIME_LINE("Full pStackArray");
        return;
    }

    s->topIdx++;
    s->array[s->topIdx] = e;
}

/******************************************************
 * 函数功能: 读取栈顶元素, 不弹出栈顶
 * ---------------------------------------------------
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 栈顶元素
 ******************************************************/
static elementType topArray(pStackArray s)
{
    if (!isEmptyArray(s))
    {
        return s->array[s->topIdx];
    }

    DEBUG_TIME_LINE("Empty pStackArray");
    exit(1);
}

/******************************************************
 * 函数功能: 弹出栈顶元素
 * ---------------------------------------------------
 * @param - s, 栈指针
 * ---------------------------------------------------
 * @return - 栈顶元素
 ******************************************************/
static elementType popArray(pStackArray s)
{
    if (isEmptyArray(s))
    {
        DEBUG_TIME_LINE("Empty pStackArray");
        exit(1);
    }

    elementType tmp = s->array[s->topIdx];
    s->topIdx--;

    return tmp;
}

/******************************************************
 * 函数功能: 创建1个栈
 * ---------------------------------------------------
 * @param - capacity, 栈的容量
 * ---------------------------------------------------
 * @return - 栈指针
 ******************************************************/
pStackArray createStackArray(int capacity)
{
    pStackArray s;

    if (capacity < MIN_STACK_ARRAY_SIZE)
    {
        capacity = MIN_STACK_ARRAY_SIZE;
        DEBUG_TIME_LINE("pStackArray size is too small, now set size to %d", MIN_STACK_ARRAY_SIZE);
    }

    s = malloc(sizeof(stackArray_s));
    if (s == NULL)
    {
        DEBUG_TIME_LINE("Out of space!!!");
        return NULL;
    }

    s->array = malloc(sizeof(elementType) * capacity);
    if (s->array == NULL)
    {
        DEBUG_TIME_LINE("Out of space!!!");
        free(s);
        return NULL;
    }

    s->capacity = capacity;
    makeEmptyArray(s);

    s->makeEmpty = makeEmptyArray;
    s->isEmpty = isEmptyArray;
    s->isFull = isFullArray;
    s->dispose = disposeStackArray;
    s->push = pushArray;
    s->top = topArray;
    s->pop = popArray;

    return s;
}

//check whether the symbol is operator?
int isTokenOperator(Token *t)
{
    switch (t->type)
    {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_BIT_OR:
        case TOKEN_BIT_AND:
        case TOKEN_BIT_XOR:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
        case TOKEN_SIN:
        case TOKEN_COS:
        case TOKEN_LPAREN:
        case TOKEN_RPAREN:
            return 1;
            break;
        default:
            return 0;
    }
}

int tokenPrecedence(Token *t)
{
    switch (t->type)
    {
        case TOKEN_START:
        case TOKEN_END:
            return 0;
        case TOKEN_LPAREN:
        case TOKEN_RPAREN:
            return 1;
        case TOKEN_BIT_OR:
        case TOKEN_BIT_AND:
        case TOKEN_BIT_XOR:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            return 2;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 3;
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
            return 4;
        case TOKEN_SIN:
        case TOKEN_COS:
            return 5;
        case TOKEN_EXPONENTIAL:
            return 6;
        default:
            return 0;
    }

    return 0;
}

//converts infix expression to postfix
void tokenConvert(Token *infix, u32 inCount, Token *postfix, pStackArray stack)
{
    int i, j = 0;
    Token t;

    for (i = 0; i < inCount; i++)
    {
        t = infix[i];
        if (t.type == TOKEN_END)
        {
            break;
        }

        if (isTokenOperator(&t) == 0)
        {
            postfix[j] = t;
            j++;
        }
        else
        {
            if (t.type == TOKEN_LPAREN)
            {
                stack->push(t, stack);
            }
            else
            {
                if (t.type == TOKEN_RPAREN)
                {
                    while (stack->top(stack).type != TOKEN_LPAREN)
                    {
                        postfix[j] = stack->pop(stack);
                        j++;
                    }

                    stack->pop(stack); //pop TOKEN_RPAREN
                }
                else
                {
                    Token s = stack->top(stack);
                    if (tokenPrecedence(&t) > tokenPrecedence(&s))
                    {
                        stack->push(t, stack);
                    }
                    else
                    {
                        while (tokenPrecedence(&t) <= tokenPrecedence(&s))
                        {
                            postfix[j] = stack->pop(stack);
                            j++;
                            s = stack->top(stack);
                        }

                        stack->push(t, stack);
                    }
                }
            }
        }
    }

    while (stack->top(stack).type != TOKEN_START)
    {
        postfix[j] = stack->pop(stack);
        j++;
    }

    stack->pop(stack);
    postfix[j].type = TOKEN_END;
}

const char* getTokenType(TokenType t)
{
    switch (t)
    {
        case TOKEN_START:
            return ("TOKEN_START\t");
        case TOKEN_INTEGER:
            return ("TOKEN_INTEGER\t");
        case TOKEN_FLOAT:
            return ("TOKEN_FLOAT\t");
        case TOKEN_PLUS:
            return ("TOKEN_PLUS\t");
        case TOKEN_MINUS:
            return ("TOKEN_MINUS\t");
        case TOKEN_MULTIPLY:
            return ("TOKEN_MULTIPLY\t");
        case TOKEN_DIVIDE:
            return ("TOKEN_DIVIDE\t");
        case TOKEN_BIT_OR:
            return ("TOKEN_BIT_OR\t");
        case TOKEN_BIT_AND:
            return ("TOKEN_BIT_AND\t");
        case TOKEN_BIT_XOR:
            return ("TOKEN_BIT_XOR\t");
        case TOKEN_LEFT_SHIFT:
            return ("TOKEN_LEFT_SHIFT\t");
        case TOKEN_RIGHT_SHIFT:
            return ("TOKEN_RIGHT_SHIFT\t");
        case TOKEN_SIN:
            return ("TOKEN_SIN\t");
        case TOKEN_COS:
            return ("TOKEN_COS\t");
        case TOKEN_LPAREN:
            return ("TOKEN_LPAREN\t");
        case TOKEN_RPAREN:
            return ("TOKEN_RPAREN\t");
        case TOKEN_REALDB:
            return ("TOKEN_REALDB\t");
        case TOKEN_END:
            return ("TOKEN_END\t\n");
        default:
            break;
    }

    return "";
}

void printTokens(Token *tokens, u32 count)
{
    int i = 0;
    for (i = 0; i < count; i++)
    {
        printf("id: %d\t, type: %s", i, getTokenType(tokens[i].type));

        if (tokens[i].type == TOKEN_END)
        {
            break;
        }

        printf("\tposition: %u\tstr: %s", tokens[i].pos + 1, tokens[i].str);
        if (tokens[i].type == TOKEN_INTEGER)
        {
            printf("\tvalue: %d", tokens[i].value.intValue);
        }
        else if (tokens[i].type == TOKEN_FLOAT)
        {
            printf("\tvalue: %f", tokens[i].value.numValue);
        }

        printf("\n");
    }
}

double getRealDB(int realNo)
{
    return 11.12;
}

double tokenEvaluate(Token *postfix, pStackArray stack)
{
    Token t, o1, o2, result;
    double operandDouble1, operandDouble2;
    int operandInt1, operandInt2;

    int i = 0;
    for (i = 0, t = postfix[0]; t.type != TOKEN_END; i++, t = postfix[i])
    {
        t = postfix[i];
        DEBUG_TIME_LINE("token: type-%s, str-%s", getTokenType(t.type), t.str);
        if (t.type == TOKEN_INTEGER || t.type == TOKEN_FLOAT)
        {
            stack->push(t, stack);
        }
        else if (t.type == TOKEN_REALDB)
        {
            result.type = TOKEN_FLOAT;
            result.value.numValue = getRealDB(atol(t.str));
            stack->push(result, stack);
        }
        else
        {
            o2 = stack->pop(stack);
            if (o2.type == TOKEN_INTEGER)
            {
                operandInt2 = o2.value.intValue;
                operandDouble2 = (double) o2.value.intValue;
            }
            else
            {
                operandDouble2 = o2.value.numValue;
            }

            o1 = stack->pop(stack);
            if (o1.type == TOKEN_INTEGER)
            {
                operandInt1 = o1.value.intValue;
                operandDouble1 = (double) o1.value.intValue;
            }
            else
            {
                operandDouble1 = o1.value.numValue;
            }

            switch (t.type)
            {
                case TOKEN_PLUS:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = operandDouble1 + operandDouble2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_MINUS:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = operandDouble1 - operandDouble2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_MULTIPLY:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = operandDouble1 * operandDouble2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_DIVIDE:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = operandDouble1 / operandDouble2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_OR:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = operandInt1 | operandInt2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %d", operandDouble1, operandDouble2, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_AND:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = operandInt1 & operandInt2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %d", operandDouble1, operandDouble2, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_XOR:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = operandInt1 ^ operandInt2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %d", operandDouble1, operandDouble2, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LEFT_SHIFT:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = operandInt1 << operandInt2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %d", operandDouble1, operandDouble2, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_RIGHT_SHIFT:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = operandInt1 >> operandInt2;
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %d", operandDouble1, operandDouble2, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_SIN:
                    break;
                case TOKEN_COS:
                    break;
                default:
                    break;
            }
        }
    }

    return stack->top(stack).value.numValue;
}

void ariMain(void)
{
    char *input = "(2.5 + 3) * 4.2 - 10.1 / #201 + (8  | 4) + (1<<3) + (16 >> 2) + (7&3)";

    Token *tokens = calloc(strlen(input) + 1, sizeof(Token));
    u32 count = tokenizer(input, tokens);

    DEBUG_TIME_LINE("-----------------after tokenizer:--------------------\n");
    printTokens(tokens, count);

    Token *postfix = calloc(count + 1, sizeof(Token));
    pStackArray stack = createStackArray(count + 1); //allocate one more for TOKEN_START

    Token start = { .type = TOKEN_START };
    stack->push(start, stack);

    tokenConvert(tokens, count, postfix, stack);

    DEBUG_TIME_LINE("-----------------after convertion:--------------------\n");
    printTokens(postfix, count);

    DEBUG_TIME_LINE("result: %f\n", tokenEvaluate(postfix, stack));
}
