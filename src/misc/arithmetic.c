#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TOKEN_STRING_MAX_SIZE  128

// Token types
typedef enum {
    TOKEN_INVALID = -1,         //无效标记
    TOKEN_START = 0,            //开始标记
    TOKEN_INTEGER,              //整数
    TOKEN_FLOAT,                //浮点数
    TOKEN_REALDB,               //实时库值
    TOKEN_STRING,               //字符串
    TOKEN_PLUS,                 //'+', 加号
    TOKEN_MINUS,                //'-', 减号
    TOKEN_MULTIPLY,             //'*', 乘号
    TOKEN_DIVIDE,               //'/', 除号
    TOKEN_BIT_OR,               //'|', 按位或
    TOKEN_BIT_AND,              //'&', 按位与
    TOKEN_BIT_XOR,              //'^', 按位异或
    TOKEN_BIT_NEGATION,         //'~', 按位取反
    TOKEN_LEFT_SHIFT,           //"<<", 左移
    TOKEN_RIGHT_SHIFT,          //">>", 右移
    TOKEN_SIN,                  //"sin", 正弦
    TOKEN_COS,                  //"cos", 余弦
    TOKEN_EXPONENTIAL,          //"exp", 指数
    TOKEN_LPAREN,               //'(', 左括号
    TOKEN_RPAREN,               //')', 右括号
    TOKEN_LOGICAL_AND,          //"&&", 逻辑与
    TOKEN_LOGICAL_OR,           //"||", 逻辑或
    TOKEN_LOGICAL_NOT,          //"!", 逻辑非
    TOKEN_LOGICA_EQUAL,         //"==", 等于
    TOKEN_LOGICA_NOT_EQUAL,     //"!=", 不等于
    TOKEN_LOGICA_GREATER,       //">", 大于
    TOKEN_LOGICA_LESS,          //"<", 小于
    TOKEN_LOGICA_GREATER_EQUAL, //">=", 大于等于
    TOKEN_LOGICA_LESS_EQUAL,    //"<=", 小于等于
    TOKEN_COMMA,                //","
    TOKEN_ASSIGN,               //"=", 赋值
    TOKEN_SEMICOLON,            //";"
    TOKEN_END                   //结束标记
} TokenType;

// Token struct
typedef struct {
    TokenType type;
    u32 id;
    char str[TOKEN_STRING_MAX_SIZE];
    unsigned int pos;
    union {
        double numValue;
        int intValue;
    } value;
} Token;

typedef struct tokenState {
    const char *start;
    const char *next;
    Token currentToken;
} tokenState_s, *pTokenState;

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
    elementType tmp = { 0 };

    if (!isEmptyArray(s))
    {
        return s->array[s->topIdx];
    }

    DEBUG_TIME_LINE("Empty pStackArray");
    tmp.type = TOKEN_INVALID;
    return tmp;
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
    elementType tmp = { 0 };

    if (isEmptyArray(s))
    {
        DEBUG_TIME_LINE("Empty pStackArray");
        tmp.type = TOKEN_INVALID;
        return tmp;
    }

    tmp = s->array[s->topIdx];
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

/******************************************************
 * 函数功能: 判断一个符号是不是操作符
 * ---------------------------------------------------
 * @param[in] - t, 符号
 * ---------------------------------------------------
 * @return - 是操作符, 返回1; 否则返回0
 ******************************************************/
int isTokenOperator(Token *t)
{
    switch (t->type)
    {
        case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_MULTIPLY:
            case TOKEN_DIVIDE:
            case TOKEN_BIT_NEGATION:
            case TOKEN_BIT_OR:
            case TOKEN_BIT_AND:
            case TOKEN_BIT_XOR:
            case TOKEN_LEFT_SHIFT:
            case TOKEN_RIGHT_SHIFT:
            case TOKEN_SIN:
            case TOKEN_COS:
            case TOKEN_LPAREN:
            case TOKEN_RPAREN:
            case TOKEN_EXPONENTIAL:
            case TOKEN_LOGICAL_AND:
            case TOKEN_LOGICAL_OR:
            case TOKEN_LOGICAL_NOT:
            case TOKEN_LOGICA_EQUAL:
            case TOKEN_LOGICA_NOT_EQUAL:
            case TOKEN_LOGICA_GREATER:
            case TOKEN_LOGICA_LESS:
            case TOKEN_LOGICA_GREATER_EQUAL:
            case TOKEN_LOGICA_LESS_EQUAL:
            case TOKEN_ASSIGN:
            case TOKEN_COMMA:
            case TOKEN_SEMICOLON:
            return 1;
            break;
        default:
            return 0;
    }
}

/******************************************************
 * 函数功能: 将算术表达式转换为符号序列
 * ---------------------------------------------------
 * @param[in] - input, 算术表达式字符串, 以'\0'结尾
 * @param[out] - tokens, 输出的符号序列
 * ---------------------------------------------------
 * @return - 输出的符号的个数
 ******************************************************/
u32 tokenizer(const char *input, Token *tokens)
{
    //todo: 改成一个个词素解析并交给求值函数
    //如果当前参与计算的操作数没有实时库，
    //则直接将求值结果压入逆波兰表达式
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

        if (input[position] == '#')         //实时库
        {
            if (position >= inputlen || input[position + 1] < '0' || input[position + 1] > '9')
            {
                printf("Invalid character: %c, position: %u\n", input[position], position);
                return 0;
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

            pToken->id = tokenCount++;
            pToken->str[i] = '\0';
            pToken->type = TOKEN_REALDB;
            pToken->value.intValue = atol(pToken->str);

            pToken++;
        }
        else if (input[position] >= '0' && input[position] <= '9') //整数或者小数
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

            pToken->id = tokenCount++;
            pToken++;
        }
        else        //操作符
        {
            switch (input[position])
            {
                case '+':
                    pToken->type = TOKEN_PLUS;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '-':
                    {
                    // 判断当前'-'是负号还是减法运算符
                    int isNegative = 0;
                    if (tokenCount == 0)
                    {
                        // 情况1：位于表达式开头，视为负号
                        isNegative = 1;
                    }
                    else
                    {
                        // 情况2：前面是左括号或其他运算符，视为负号
                        if (isTokenOperator(pToken - 1))
                        {
                            isNegative = 1;
                        }
                    }

                    if (isNegative)
                    {
                        // 处理负数（负号+数字）
                        pToken->pos = position;  // 记录负号位置
                        int i = 0;
                        pToken->str[i++] = '-';  // 保存负号到字符串

                        position++;  // 跳过负号，解析后续数字

                        // 检查负号后是否有数字（避免无效格式如"-abc"）
                        if (position >= inputlen || input[position] < '0' || input[position] > '9')
                        {
                            printf("Invalid negative number: '-' at position %u has no digit following\n", position - 1);
                            return 0;
                        }

                        // 解析整数部分
                        while (input[position] >= '0' && input[position] <= '9')
                        {
                            pToken->str[i++] = input[position];
                            position++;
                        }

                        // 解析小数部分（如果有）
                        int hasDecimal = 0;
                        if (input[position] == '.')
                        {
                            hasDecimal = 1;
                            pToken->str[i++] = '.';
                            position++;

                            // 检查小数点后是否有数字（避免无效格式如"-123."）
                            if (position >= inputlen || input[position] < '0' || input[position] > '9')
                            {
                                printf("Invalid decimal part in negative number at position %u\n", position - 1);
                                return 0;
                            }

                            while (input[position] >= '0' && input[position] <= '9')
                            {
                                pToken->str[i++] = input[position];
                                position++;
                            }
                        }

                        pToken->str[i] = '\0';  // 字符串结束符

                        // 设置token类型和值（负数）
                        if (hasDecimal)
                        {
                            pToken->type = TOKEN_FLOAT;
                            pToken->value.numValue = atof(pToken->str);  // atof自动处理负号
                        }
                        else
                        {
                            pToken->type = TOKEN_INTEGER;
                            pToken->value.intValue = atol(pToken->str);  // atol自动处理负号
                        }

                        pToken->id = tokenCount++;
                        pToken++;
                    }
                    else
                    {
                        // 处理减法运算符
                        pToken->type = TOKEN_MINUS;
                        pToken->pos = position;
                        pToken->str[0] = '-';
                        pToken->str[1] = '\0';

                        position++;
                        pToken->id = tokenCount++;
                        pToken++;
                    }
                }
                    break;
                case '*':
                    pToken->type = TOKEN_MULTIPLY;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '/':
                    pToken->type = TOKEN_DIVIDE;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '|':
                    if (position < inputlen - 1 && input[position + 1] == '|')
                    {
                        pToken->type = TOKEN_LOGICAL_OR;
                        pToken->pos = position;
                        strncpy(pToken->str, "||", sizeof(pToken->str) - 1);
                        position++;
                    }
                    else
                    {
                        pToken->type = TOKEN_BIT_OR;
                        pToken->pos = position;
                        pToken->str[0] = input[position];
                        pToken->str[1] = '\0';
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '&':
                    if (position < inputlen - 1 && input[position + 1] == '&')
                    {
                        pToken->type = TOKEN_LOGICAL_AND;
                        pToken->pos = position;
                        strncpy(pToken->str, "&&", sizeof(pToken->str) - 1);
                        position++;
                    }
                    else
                    {
                        pToken->type = TOKEN_BIT_AND;
                        pToken->pos = position;
                        pToken->str[0] = input[position];
                        pToken->str[1] = '\0';
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '^':
                    pToken->type = TOKEN_BIT_XOR;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '~':
                    pToken->type = TOKEN_BIT_NEGATION;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '<':
                    if (position < inputlen - 1)
                    {
                        if (input[position + 1] == '<')
                        {
                            pToken->type = TOKEN_LEFT_SHIFT;
                            pToken->pos = position;
                            strncpy(pToken->str, "<<", sizeof(pToken->str) - 1);
                            position++;
                        }
                        else if (input[position + 1] == '=')
                        {
                            pToken->type = TOKEN_LOGICA_LESS_EQUAL;
                            pToken->pos = position;
                            strncpy(pToken->str, "<=", sizeof(pToken->str) - 1);
                            position++;
                        }
                        else
                        {
                            pToken->type = TOKEN_LOGICA_LESS;
                            pToken->pos = position;
                            pToken->str[0] = input[position];
                            pToken->str[1] = '\0';
                        }
                    }
                    else
                    {
                        pToken->type = TOKEN_LOGICA_LESS;
                        pToken->pos = position;
                        pToken->str[0] = input[position];
                        pToken->str[1] = '\0';
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '>':
                    if (position < inputlen - 1)
                    {
                        if (input[position + 1] == '>')
                        {
                            pToken->type = TOKEN_RIGHT_SHIFT;
                            pToken->pos = position;
                            strncpy(pToken->str, ">>", sizeof(pToken->str) - 1);
                            position++;
                        }
                        else if (input[position + 1] == '=')
                        {
                            pToken->type = TOKEN_LOGICA_GREATER_EQUAL;
                            pToken->pos = position;
                            strncpy(pToken->str, ">=", sizeof(pToken->str) - 1);
                            position++;
                        }
                        else
                        {
                            pToken->type = TOKEN_LOGICA_GREATER;
                            pToken->pos = position;
                            pToken->str[0] = input[position];
                            pToken->str[1] = '\0';
                        }
                    }
                    else
                    {
                        pToken->type = TOKEN_LOGICA_GREATER;
                        pToken->pos = position;
                        pToken->str[0] = input[position];
                        pToken->str[1] = '\0';
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '=':
                    if (position < inputlen - 1 && input[position + 1] == '=')
                    {
                        pToken->type = TOKEN_LOGICA_EQUAL;
                        pToken->pos = position;
                        strncpy(pToken->str, "==", sizeof(pToken->str) - 1);
                        position++;
                    }
                    else
                    {
                        pToken->type = TOKEN_ASSIGN;
                        pToken->pos = position;
                        pToken->str[0] = input[position];
                        pToken->str[1] = '\0';
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '!':
                    if (position < inputlen - 1 && input[position + 1] == '=')
                    {
                        pToken->type = TOKEN_LOGICA_NOT_EQUAL;
                        pToken->pos = position;
                        strncpy(pToken->str, "!=", sizeof(pToken->str) - 1);
                        position++;
                    }
                    else
                    {
                        pToken->type = TOKEN_LOGICAL_NOT;
                        pToken->pos = position;
                        pToken->str[0] = input[position];
                        pToken->str[1] = '\0';
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case '(':
                    pToken->type = TOKEN_LPAREN;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case ')':
                    pToken->type = TOKEN_RPAREN;
                    pToken->pos = position;
                    pToken->str[0] = input[position];
                    pToken->str[1] = '\0';

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
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
                        return 0;
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
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
                        return 0;
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                case 'e':
                    if (position < inputlen - 2 && input[position + 1] == 'x' && input[position + 2] == 'p')
                    {
                        pToken->type = TOKEN_EXPONENTIAL;
                        pToken->pos = position;
                        strncpy(pToken->str, "exp", sizeof(pToken->str) - 1);
                        position += 2;
                    }
                    else
                    {
                        printf("Invalid character: %c, position: %u\n", input[position], position);
                        return 0;
                    }

                    position++;

                    pToken->id = tokenCount++;
                    pToken++;
                    break;
                default:
                    printf("Invalid character: %c\n", input[position]);
                    return 0;
            }
        }
    }

    pToken->type = TOKEN_END;
    pToken->id = tokenCount++;

    return tokenCount;
}

/******************************************************
 * 函数功能: 将算术表达式转换为符号序列
 * ---------------------------------------------------
 * @param[in] - input, 算术表达式字符串, 以'\0'结尾
 * @param[out] - tokens, 输出的符号序列
 * ---------------------------------------------------
 * @return - 输出的符号的个数
 ******************************************************/
void getNextToken(pTokenState s, Token *tokens)
{
    s->currentToken.type = TOKEN_INVALID;
}

/******************************************************
 * 函数功能: 输出一个操作符的优先级
 * ---------------------------------------------------
 * @param[in] - t, 符号
 * ---------------------------------------------------
 * @return - 操作符的优先级
 ******************************************************/
int tokenPrecedence(Token *t)
{
    switch (t->type)
    {
        case TOKEN_START:
            case TOKEN_END:
            return 0;
        case TOKEN_ASSIGN:
            return 1;
        case TOKEN_LOGICAL_OR:
            return 2;
        case TOKEN_LOGICAL_AND:
            return 3;
        case TOKEN_BIT_OR:
            return 4;
        case TOKEN_BIT_XOR:
            return 5;
        case TOKEN_BIT_AND:
            return 6;
        case TOKEN_LOGICA_NOT_EQUAL:
            case TOKEN_LOGICA_EQUAL:
            return 7;
        case TOKEN_LOGICA_GREATER:
            case TOKEN_LOGICA_LESS:
            case TOKEN_LOGICA_GREATER_EQUAL:
            case TOKEN_LOGICA_LESS_EQUAL:
            return 8;
        case TOKEN_LEFT_SHIFT:
            case TOKEN_RIGHT_SHIFT:
            return 9;
        case TOKEN_PLUS:
            case TOKEN_MINUS:
            return 10;
        case TOKEN_MULTIPLY:
            case TOKEN_DIVIDE:
            return 11;
        case TOKEN_LOGICAL_NOT:
            case TOKEN_BIT_NEGATION:
            return 12;
        case TOKEN_SIN:
            case TOKEN_COS:
            return 13;
        case TOKEN_EXPONENTIAL:
            return 14;
        default:
            return 0;
    }

    return 0;
}

/******************************************************
 * 函数功能: 将算术表达式转换为逆波兰表达式(即后缀表达式)
 * ---------------------------------------------------
 * @param[in] - infix, 中缀算术表达式序列,
 *              以(TOKEN_END)结尾
 * @param[in] - inCount, 中缀算术表达式序列的长度
 * @param[out] - postfix, 输出的逆波兰表达式
 * @param[in] - stack, 计算过程中用到的栈, 用于存储操作符
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void tokenConvert(Token *infix, u32 inCount, Token *postfix, pStackArray stack)
{
    int i, j = 0;
    Token t;

    for (i = 0; i < inCount; i++)
    {
        t = infix[i];
        if (t.type == TOKEN_END) //遇到结束符号, 退出
        {
            break;
        }

        if (t.type == TOKEN_INTEGER || t.type == TOKEN_FLOAT || t.type == TOKEN_REALDB) //操作数, 直接压栈
        {
            postfix[j] = t;
            j++;
        }
        else if (t.type == TOKEN_LPAREN) //左括号, 直接压栈
        {
            stack->push(t, stack);
        }
        else if (t.type == TOKEN_RPAREN) //右括号, 将左括号之前的元素都出栈
        {
            while (stack->top(stack).type != TOKEN_LPAREN)
            {
                postfix[j] = stack->pop(stack);
                j++;
            }

            stack->pop(stack); //弹出左括号
        }
        else //其他操作符的处理
        {
            Token s = stack->top(stack); //取栈顶元素
            if (tokenPrecedence(&t) > tokenPrecedence(&s))
            {
                //如果当前操作符的优先级高于栈顶操作符, 则将当前操作符直接压栈
                stack->push(t, stack);
            }
            else
            {
                /*
                 * 如果当前操作符的优先级不高于栈顶操作符,
                 * 则将栈中的高于或等于当前操作符优先级的操作符都出栈,
                 * 然后将当前操作符压栈
                 */
                while (tokenPrecedence(&t) <= tokenPrecedence(&s))
                {
                    postfix[j] = stack->pop(stack);
                    j++;
                    s = stack->top(stack);
                }

                stack->push(t, stack); //将当前操作符压栈
            }
        }
    }

    //将栈中剩下的元素出栈
    while (stack->top(stack).type != TOKEN_START)
    {
        postfix[j] = stack->pop(stack);
        j++;
    }

    stack->pop(stack); //将 TOKEN_START 出栈
    postfix[j].type = TOKEN_END; //将最后一个元素标记为 TOKEN_END
}

/******************************************************
 * 函数功能: 输出一个符号的类型字符串
 * ---------------------------------------------------
 * @param[in] - t, 符号的类型
 * ---------------------------------------------------
 * @return - 符号的类型字符串
 ******************************************************/
const char* getTokenType(TokenType t)
{
    switch (t)
    {
        case TOKEN_START:
            return ("TOKEN_START");
        case TOKEN_INTEGER:
            return ("TOKEN_INTEGER");
        case TOKEN_FLOAT:
            return ("TOKEN_FLOAT");
        case TOKEN_PLUS:
            return ("TOKEN_PLUS");
        case TOKEN_MINUS:
            return ("TOKEN_MINUS");
        case TOKEN_MULTIPLY:
            return ("TOKEN_MULTIPLY");
        case TOKEN_DIVIDE:
            return ("TOKEN_DIVIDE");
        case TOKEN_BIT_OR:
            return ("TOKEN_BIT_OR");
        case TOKEN_BIT_AND:
            return ("TOKEN_BIT_AND");
        case TOKEN_BIT_XOR:
            return ("TOKEN_BIT_XOR");
        case TOKEN_LEFT_SHIFT:
            return ("TOKEN_LEFT_SHIFT");
        case TOKEN_RIGHT_SHIFT:
            return ("TOKEN_RIGHT_SHIFT");
        case TOKEN_SIN:
            return ("TOKEN_SIN");
        case TOKEN_COS:
            return ("TOKEN_COS");
        case TOKEN_EXPONENTIAL:
            return ("TOKEN_EXPONENTIAL");
        case TOKEN_LPAREN:
            return ("TOKEN_LPAREN");
        case TOKEN_RPAREN:
            return ("TOKEN_RPAREN");
        case TOKEN_REALDB:
            return ("TOKEN_REALDB");
        case TOKEN_STRING:
            return ("TOKEN_STRING");
        case TOKEN_LOGICAL_AND:
            return ("TOKEN_LOGICAL_AND");
        case TOKEN_LOGICAL_OR:
            return ("TOKEN_LOGICAL_OR");
        case TOKEN_LOGICAL_NOT:
            return ("TOKEN_LOGICAL_NOT");
        case TOKEN_LOGICA_EQUAL:
            return ("TOKEN_LOGICA_EQUAL");
        case TOKEN_LOGICA_NOT_EQUAL:
            return ("TOKEN_LOGICA_NOT_EQUAL");
        case TOKEN_LOGICA_GREATER:
            return ("TOKEN_GREATER");
        case TOKEN_LOGICA_LESS:
            return ("TOKEN_LESS");
        case TOKEN_LOGICA_GREATER_EQUAL:
            return ("TOKEN_GREATER_EQUAL");
        case TOKEN_LOGICA_LESS_EQUAL:
            return ("TOKEN_LESS_EQUAL");
        case TOKEN_COMMA:
            return ("TOKEN_COMMA");
        case TOKEN_ASSIGN:
            return ("TOKEN_ASSIGN");
        case TOKEN_SEMICOLON:
            return ("TOKEN_SEMICOLON");
        case TOKEN_BIT_NEGATION:
            return ("TOKEN_BIT_NEGATION");
        case TOKEN_END:
            return ("TOKEN_END");
        default:
            break;
    }

    return "NOT DEFINED TOKEN";
}

/******************************************************
 * 函数功能: 打印符号序列
 * ---------------------------------------------------
 * @param[in] - tokens, 符号序列
 * @param[in] - count, 符号序列的长度
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void printTokens(Token *tokens, u32 count)
{
    int i = 0;
    for (i = 0; i < count && tokens[i].type != TOKEN_END; i++)
    {
        printf("id: %u\t, type: %s,\t\t", tokens[i].id, getTokenType(tokens[i].type));

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

/******************************************************
 * 函数功能: 获取实时库的值
 * ---------------------------------------------------
 * @param[in] - realNo, 实时库号
 * ---------------------------------------------------
 * @return - 实时库的值
 ******************************************************/
double getRealDB(int realNo)
{
    return 11.12;
}

/******************************************************
 * 函数功能: 设置实时库的值
 * ---------------------------------------------------
 * @param[in] - realNo, 实时库号
 * @param[in] - value, 设置的值
 * ---------------------------------------------------
 * @return - 0 - 成功; -1 - 失败
 ******************************************************/
int setRealDB(int realNo, double value)
{
    DEBUG_TIME_LINE("realNo: %d, value: %f", realNo, value);

    return 0;
}

/******************************************************
 * 函数功能: 对逆波兰表达式求值
 * ---------------------------------------------------
 * @param[in] - postfix, 逆波兰表达式
 * @param[in] - stack, 计算用到的临时栈
 * ---------------------------------------------------
 * @return - 求值结果
 ******************************************************/
double tokenEvaluate(Token *postfix, pStackArray stack)
{
    Token t, o1, o2, result;
    double operandDouble1, operandDouble2;

    int i = 0;
    for (i = 0, t = postfix[0]; t.type != TOKEN_END; i++, t = postfix[i])
    {
        t = postfix[i];
        DEBUG_TIME_LINE("token: type-%s, str: %s", getTokenType(t.type), t.str);
        if (t.type == TOKEN_INTEGER ||
                t.type == TOKEN_FLOAT ||
                t.type == TOKEN_REALDB)
        {
            //操作数直接压栈
            stack->push(t, stack);
        }
        else if (t.type == TOKEN_SIN || t.type == TOKEN_COS ||
                t.type == TOKEN_BIT_NEGATION || t.type == TOKEN_LOGICAL_NOT)
        { //单目运算符
            o1 = stack->pop(stack);
            if (o1.type == TOKEN_INTEGER)
            {
                operandDouble1 = (double) o1.value.intValue;
            }
            else if (o1.type == TOKEN_REALDB)
            {
                operandDouble1 = getRealDB(o1.value.intValue);
            }
            else
            {
                operandDouble1 = o1.value.numValue;
            }

            switch (t.type)
            {
                case TOKEN_SIN:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = sin(operandDouble1);
                    DEBUG_TIME_LINE("operand1: %f, result: %f", operandDouble1, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_COS:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = cos(operandDouble1);
                    DEBUG_TIME_LINE("operand1: %f, result: %f", operandDouble1, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_NEGATION:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (~(int) operandDouble1);
                    DEBUG_TIME_LINE("operand1: %d, result: %d", (int )operandDouble1, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICAL_NOT:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (!(int) operandDouble1);
                    DEBUG_TIME_LINE("operand1: %d, result: %d", (int )operandDouble1, result.value.intValue);
                    stack->push(result, stack);
                    break;
                default:
                    break;
            }
        }
        else //双目运算符
        {
            o2 = stack->pop(stack);
            if (o2.type == TOKEN_INTEGER)
            {
                operandDouble2 = (double) o2.value.intValue;
            }
            else if (o2.type == TOKEN_REALDB)
            {
                operandDouble2 = getRealDB(o2.value.intValue);
            }
            else
            {
                operandDouble2 = o2.value.numValue;
            }

            o1 = stack->pop(stack);
            if (o1.type == TOKEN_INTEGER)
            {
                operandDouble1 = (double) o1.value.intValue;
            }
            else if (o1.type == TOKEN_REALDB)
            {
                operandDouble1 = getRealDB(o1.value.intValue);
            }
            else
            {
                operandDouble1 = o1.value.numValue;
            }

            switch (t.type)
            {
                case TOKEN_PLUS:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = (operandDouble1 + operandDouble2);
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_MINUS:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = (operandDouble1 - operandDouble2);
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_MULTIPLY:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = (operandDouble1 * operandDouble2);
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_DIVIDE:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = (operandDouble1 / operandDouble2);
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_EXPONENTIAL:
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = pow(operandDouble1, operandDouble2);
                    DEBUG_TIME_LINE("operand1: %f, operand2: %f, result: %f", operandDouble1, operandDouble2, result.value.numValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_OR:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) | ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, (int )operandDouble2, result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_AND:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) & ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_BIT_XOR:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) ^ ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LEFT_SHIFT:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) << ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_RIGHT_SHIFT:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) >> ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICAL_AND:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) && ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICAL_OR:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) || ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICA_EQUAL:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) == ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICA_NOT_EQUAL:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) != ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICA_GREATER:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) > ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICA_LESS:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) < ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICA_GREATER_EQUAL:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) >= ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_LOGICA_LESS_EQUAL:
                    result.type = TOKEN_INTEGER;
                    result.value.intValue = (((int) operandDouble1) <= ((int) operandDouble2));
                    DEBUG_TIME_LINE("operand1: %d, operand2: %d, result: %d", (int )operandDouble1, ((int )operandDouble2), result.value.intValue);
                    stack->push(result, stack);
                    break;
                case TOKEN_ASSIGN:
                    if (o1.type != TOKEN_REALDB)
                    {
                        DEBUG_TIME_LINE("only RealDatabase can be assgned a value: id: %u, type: %s, position: %d",
                                o1.id, getTokenType(o1.type), o1.pos);
                        return 0;
                    }

                    setRealDB(o1.value.intValue, operandDouble2);
                    result.type = TOKEN_FLOAT;
                    result.value.numValue = operandDouble2;
                    DEBUG_TIME_LINE("operand1: #%d, operand2: %f, result: %f", o1.value.intValue, operandDouble1, result.value.numValue);
                    stack->push(result, stack);
                    break;
                default:
                    break;
            }
        }
    }

    Token top = stack->top(stack);
    if (top.type == TOKEN_INTEGER)
    {
        return (double) top.value.intValue;
    }

    return top.value.numValue;
}

/******************************************************
 * 函数功能: 测试逆波兰表达式求值
 * ---------------------------------------------------
 * @param[in] - 无
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
typedef struct FoldNode {
    Token tok;
    int contains_real; /* whether subtree contains a TOKEN_REALDB leaf */
    struct FoldNode *left;
    struct FoldNode *right;
} FoldNode;

static FoldNode* foldnode_new_leaf(const Token *t)
{
    FoldNode *n = malloc(sizeof(FoldNode));
    n->tok = *t; /* shallow copy is fine */
    n->contains_real = (t->type == TOKEN_REALDB) ? 1 : 0;
    n->left = n->right = NULL;
    return n;
}

static FoldNode* foldnode_new_unary(const Token *op, FoldNode *child)
{
    FoldNode *n = malloc(sizeof(FoldNode));
    n->tok = *op;
    n->contains_real = child->contains_real;
    n->left = child;
    n->right = NULL;
    return n;
}

static FoldNode* foldnode_new_binary(const Token *op, FoldNode *left, FoldNode *right)
{
    FoldNode *n = malloc(sizeof(FoldNode));
    n->tok = *op;
    n->contains_real = left->contains_real || right->contains_real;
    n->left = left;
    n->right = right;
    return n;
}

static void foldnode_free(FoldNode *n)
{
    if (!n) return;
    foldnode_free(n->left);
    foldnode_free(n->right);
    free(n);
}

static int is_unary_token(TokenType t)
{
    return (t == TOKEN_SIN || t == TOKEN_COS || t == TOKEN_BIT_NEGATION || t == TOKEN_LOGICAL_NOT);
}

static int is_binary_token(TokenType t)
{
    switch (t)
    {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_EXPONENTIAL:
        case TOKEN_BIT_OR:
        case TOKEN_BIT_AND:
        case TOKEN_BIT_XOR:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
        case TOKEN_LOGICAL_AND:
        case TOKEN_LOGICAL_OR:
        case TOKEN_LOGICA_EQUAL:
        case TOKEN_LOGICA_NOT_EQUAL:
        case TOKEN_LOGICA_GREATER:
        case TOKEN_LOGICA_LESS:
        case TOKEN_LOGICA_GREATER_EQUAL:
        case TOKEN_LOGICA_LESS_EQUAL:
        case TOKEN_ASSIGN:
            return 1;
        default:
            return 0;
    }
}

/* Helper: try to evaluate a node if it contains no real DB and children are numeric.
 * On success, returns a new leaf FoldNode containing the folded constant (and frees old node).
 * On failure, returns the original node unchanged. */
static FoldNode* try_fold_node(FoldNode *n)
{
    if (!n) return NULL;
    if (n->contains_real) return n; /* cannot fold if subtree refers to real DB */

    /* If leaf already, nothing to fold */
    if (!n->left && !n->right) return n;

    /* Check children are numeric leaves */
    if (n->left && (n->left->tok.type != TOKEN_INTEGER && n->left->tok.type != TOKEN_FLOAT)) return n;
    if (n->right && (n->right->tok.type != TOKEN_INTEGER && n->right->tok.type != TOKEN_FLOAT))
    {
        /* unary case may have only left */
        if (n->right) return n;
    }

    double lval = 0.0, rval = 0.0;
    if (n->left)
    {
        if (n->left->tok.type == TOKEN_INTEGER) lval = (double)n->left->tok.value.intValue;
        else lval = n->left->tok.value.numValue;
    }
    if (n->right)
    {
        if (n->right->tok.type == TOKEN_INTEGER) rval = (double)n->right->tok.value.intValue;
        else rval = n->right->tok.value.numValue;
    }

    Token folded = {0};

    /* Evaluate following tokenEvaluate semantics, avoid undefined ops (e.g., division by zero)
     * Assignment is not folded because it is a side-effect unless we know LHS is realdb (we don't want to perform assignment at compile-time). */
    switch (n->tok.type)
    {
        case TOKEN_SIN:
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = sin(lval);
            break;
        case TOKEN_COS:
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = cos(lval);
            break;
        case TOKEN_BIT_NEGATION:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ~(int)lval;
            break;
        case TOKEN_LOGICAL_NOT:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = !((int)lval);
            break;

        case TOKEN_PLUS:
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = lval + rval;
            break;
        case TOKEN_MINUS:
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = lval - rval;
            break;
        case TOKEN_MULTIPLY:
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = lval * rval;
            break;
        case TOKEN_DIVIDE:
            if (rval == 0.0) return n; /* do not fold division by zero */
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = lval / rval;
            break;
        case TOKEN_EXPONENTIAL:
            folded.type = TOKEN_FLOAT;
            folded.value.numValue = pow(lval, rval);
            break;
        case TOKEN_BIT_OR:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) | ((int)rval);
            break;
        case TOKEN_BIT_AND:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) & ((int)rval);
            break;
        case TOKEN_BIT_XOR:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) ^ ((int)rval);
            break;
        case TOKEN_LEFT_SHIFT:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) << ((int)rval);
            break;
        case TOKEN_RIGHT_SHIFT:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) >> ((int)rval);
            break;
        case TOKEN_LOGICAL_AND:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) && ((int)rval);
            break;
        case TOKEN_LOGICAL_OR:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) || ((int)rval);
            break;
        case TOKEN_LOGICA_EQUAL:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) == ((int)rval);
            break;
        case TOKEN_LOGICA_NOT_EQUAL:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) != ((int)rval);
            break;
        case TOKEN_LOGICA_GREATER:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) > ((int)rval);
            break;
        case TOKEN_LOGICA_LESS:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) < ((int)rval);
            break;
        case TOKEN_LOGICA_GREATER_EQUAL:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) >= ((int)rval);
            break;
        case TOKEN_LOGICA_LESS_EQUAL:
            folded.type = TOKEN_INTEGER;
            folded.value.intValue = ((int)lval) <= ((int)rval);
            break;
        case TOKEN_ASSIGN:
            /* Do not fold assignment: side-effect. */
            return n;
        default:
            return n; /* unknown op: cannot fold */
    }

    /* If we reach here, we successfully folded. Create a new leaf node and free subtree. */
    FoldNode *leaf = malloc(sizeof(FoldNode));
    leaf->tok = folded;
    leaf->contains_real = 0;
    leaf->left = leaf->right = NULL;

    /* free original subtree */
    if (n->left) foldnode_free(n->left);
    if (n->right) foldnode_free(n->right);
    free(n);

    return leaf;
}

/* Convert a postfix token sequence into a folded postfix sequence.
 * postfix_in and postfix_out must be large enough; returns number of tokens written (excluding final TOKEN_END). */
static u32 optimize_postfix(Token *postfix_in, Token *postfix_out)
{
    /* We'll construct expression trees from postfix using a stack of FoldNode* */
    u32 outCount = 0;
    FoldNode **stack = calloc(256, sizeof(FoldNode*));
    int top = -1;

    for (u32 i = 0; ; ++i)
    {
        Token t = postfix_in[i];
        if (t.type == TOKEN_END) break;

        if (t.type == TOKEN_INTEGER || t.type == TOKEN_FLOAT || t.type == TOKEN_REALDB)
        {
            FoldNode *leaf = foldnode_new_leaf(&t);
            stack[++top] = leaf;
        }
        else if (is_unary_token(t.type))
        {
            if (top < 0) { /* malformed postfix */ break; }
            FoldNode *child = stack[top--];
            FoldNode *node = foldnode_new_unary(&t, child);
            /* try folding */
            FoldNode *maybe = try_fold_node(node);
            stack[++top] = maybe;
        }
        else if (is_binary_token(t.type))
        {
            if (top < 1) { /* malformed */ break; }
            FoldNode *right = stack[top--];
            FoldNode *left = stack[top--];
            FoldNode *node = foldnode_new_binary(&t, left, right);
            /* try folding */
            FoldNode *maybe = try_fold_node(node);
            stack[++top] = maybe;
        }
        else
        {
            /* unknown token: cannot handle - push as-is to output later. We'll create a leaf to preserve it. */
            FoldNode *leaf = foldnode_new_leaf(&t);
            stack[++top] = leaf;
        }

        /* grow stack if necessary */
        if (top > 200) {
            stack = realloc(stack, (top + 256) * sizeof(FoldNode*));
        }
    }

    /* Now each element on stack is a top-level expression node; emit them in postfix order. */
    /* helper recursive emitter */
    void emit_node(FoldNode *n)
    {
        if (!n) return;
        if (n->left) emit_node(n->left);
        if (n->right) emit_node(n->right);
        /* append token */
        postfix_out[outCount++] = n->tok;
    }

    for (int i = 0; i <= top; ++i)
    {
        emit_node(stack[i]);
        foldnode_free(stack[i]);
    }

    free(stack);

    /* terminate */
    postfix_out[outCount].type = TOKEN_END;
    return outCount;
}

void ariMain(void)
{
//    char *input = "(2.5 + 3) * 4.2 - 10.1 / #201 + (8  | 4) + (#1<<3) + (16 >> 2) + (7&3) + sin(12) + cos(20) + 2exp(30)+(1==2) + (1!=2)+(1<2)+(1>2)+(1<=2)+(1>=2)";
//    char *input ="56--9+-6.3--";
//    char *input = "65--11+2";
    char *input = "65--11+2*sin(12)";

    DEBUG_TIME_LINE("before tokenizer: %s", input);

    Token *tokens = calloc(strlen(input) + 1, sizeof(Token));
    u32 count = tokenizer(input, tokens);
    if (count == 0)
    {
        DEBUG_TIME_LINE("tokenizer error");
        return;
    }

    DEBUG_TIME_LINE("\n-----------------after tokenizer:--------------------\n");
    printTokens(tokens, count);
    Token *postfix = calloc(count + 1, sizeof(Token));
    pStackArray stack = createStackArray(count + 1); //allocate one more for TOKEN_START

    Token start = { .type = TOKEN_START };
    stack->push(start, stack);

    tokenConvert(tokens, count, postfix, stack);

    DEBUG_TIME_LINE("\n-----------------after convertion:--------------------\n");
    printTokens(postfix, count);

    /* New: optimize postfix by constant-folding subexpressions that do not reference real DB nodes */
    Token *opt_postfix = calloc(count + 1, sizeof(Token));
    optimize_postfix(postfix, opt_postfix);

    DEBUG_TIME_LINE("\n-----------------after optimization:--------------------\n");
    printTokens(opt_postfix, count);

    DEBUG_TIME_LINE("result: %f\n", tokenEvaluate(opt_postfix, stack));

    stack->dispose(stack);
}
