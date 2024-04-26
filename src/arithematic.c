#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_STRING_MAX_SIZE  128

// Token types
typedef enum {
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
    TOKEN_RIGHT_SIN,
    TOKEN_RIGHT_COS,
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

typedef TokenType* elementType;

struct stackArray;
typedef struct stackArray stackArray_s;
typedef stackArray_s* pStackArray;

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
                        pToken->type = TOKEN_RIGHT_SIN;
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
                        pToken->type = TOKEN_RIGHT_SIN;
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

    return NULL;
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
        return NULL;
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


void ariMain(void)
{
    char *input = "(2.5 + 3) * 4.2 - 10.1 / 2 + #1485548 >>  #6545 || 5 ^ 2 & 3<< 16.1235 + sin(#45) / cos(123)";

    Token *tokens = calloc(strlen(input) + 1, sizeof(Token));
    size_t count = tokenizer(input, tokens);

    int i = 0;
    for (i = 0; i < count; i++)
    {
        printf("id: %d\t", i);
        switch (tokens[i].type)
        {
            case TOKEN_INTEGER:
                printf("type: TOKEN_INTEGER\t");
                break;
            case TOKEN_FLOAT:
                printf("type: TOKEN_FLOAT\t");
                break;
            case TOKEN_PLUS:
                printf("type: TOKEN_PLUS\t");
                break;
            case TOKEN_MINUS:
                printf("type: TOKEN_MINUS\t");
                break;
            case TOKEN_MULTIPLY:
                printf("type: TOKEN_MULTIPLY\t");
                break;
            case TOKEN_DIVIDE:
                printf("type: TOKEN_DIVIDE\t");
                break;
            case TOKEN_BIT_OR:
                printf("type: TOKEN_BIT_OR\t");
                break;
            case TOKEN_BIT_AND:
                printf("type: TOKEN_BIT_AND\t");
                break;
            case TOKEN_BIT_XOR:
                printf("type: TOKEN_BIT_XOR\t");
                break;
            case TOKEN_LEFT_SHIFT:
                printf("type: TOKEN_LEFT_SHIFT\t");
                break;
            case TOKEN_RIGHT_SHIFT:
                printf("type: TOKEN_RIGHT_SHIFT\t");
                break;
            case TOKEN_RIGHT_SIN:
                printf("type: TOKEN_RIGHT_SIN\t");
                break;
            case TOKEN_RIGHT_COS:
                printf("type: TOKEN_RIGHT_COS\t");
                break;
            case TOKEN_LPAREN:
                printf("type: TOKEN_LPAREN\t");
                break;
            case TOKEN_RPAREN:
                printf("type: TOKEN_RPAREN\t");
                break;
            case TOKEN_REALDB:
                printf("type: TOKEN_REALDB\t");
                break;
            case TOKEN_END:
                printf("type: TOKEN_END\t\n");
                break;
            default:
                break;
        }

        if (tokens[i].type != TOKEN_END)
        {
            printf("\tposition: %u\tstr: %s", tokens[i].pos + 1, tokens[i].str);
            if (tokens[i].type == TOKEN_INTEGER)
            {
                printf("\tvalue: %d\n", tokens[i].value.intValue);
            }
            else if (tokens[i].type == TOKEN_FLOAT)
            {
                printf("\tvalue: %f\n", tokens[i].value.numValue);
            }
            else
            {
                printf("\n");
            }
        }
    }
}
