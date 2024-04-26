#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_MAX_SIZE  128
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
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_REALDB,
    TOKEN_END
} TokenType;

// Token struct
typedef struct {
    TokenType type;
    char str[TOKEN_MAX_SIZE];
    unsigned int pos;
    union {
        double numValue;
        int intValue;
    } value;
} Token;

// Global variables
Token currentToken;
char *input;
int position = 0;

// Function prototypes
Token getNextToken();
double parseExpression();
double parseTerm();
double parseFactor();

Token getNextToken()
{
    while (input[position] != '\0')
    {
        if (input[position] == ' ')
        {
            position++;
            continue;
        }

        if (input[position] >= '0' && input[position] <= '9')
        {
            char *value = malloc(sizeof(char) * 100);
            int i = 0;

            while (input[position] >= '0' && input[position] <= '9')
            {
                value[i] = input[position];
                position++;
                i++;
            }

            if (input[position] == '.')
            {
                value[i] = input[position];
                position++;
                i++;

                while (input[position] >= '0' && input[position] <= '9')
                {
                    value[i] = input[position];
                    position++;
                    i++;
                }

                value[i] = '\0';
                currentToken.type = TOKEN_FLOAT;
            }
            else
            {
                value[i] = '\0';
                currentToken.type = TOKEN_INTEGER;
            }

            strncpy(currentToken.str, value, sizeof(currentToken.str) - 1);
            return currentToken;
        }

        switch (input[position])
        {
            case '+':
                currentToken.type = TOKEN_PLUS;
                break;
            case '-':
                currentToken.type = TOKEN_MINUS;
                break;
            case '*':
                currentToken.type = TOKEN_MULTIPLY;
                break;
            case '/':
                currentToken.type = TOKEN_DIVIDE;
                break;
            case '(':
                currentToken.type = TOKEN_LPAREN;
                break;
            case ')':
                currentToken.type = TOKEN_RPAREN;
                break;
            default:
                printf("Invalid character: %c\n", input[position]);
                exit(1);
        }

        position++;
        return currentToken;
    }

    currentToken.type = TOKEN_END;
    return currentToken;
}

Token getNextTokenMy()
{
    size_t inputlen = strlen(input);
    while (input[position] != '\0')
    {
        if (input[position] == ' ' || input[position] == '\t' || input[position] == '\r' || input[position] == '\n')
        {
            position++;
            continue;
        }

        if (input[position] == '#')
        {
            if (position >= inputlen || input[position + 1] < '0' || input[position + 1] > '9')
            {
                printf("Invalid character: %c, position: %d\n", input[position], position);
                exit(1);
            }

            currentToken.pos = position;
            position++;

            int i = 0;
            while (input[position] >= '0' && input[position] <= '9')
            {
                currentToken.str[i] = input[position];
                position++;
                i++;
            }

            currentToken.str[i] = '\0';
            currentToken.type = TOKEN_REALDB;
            currentToken.value.intValue = atol(currentToken.str);

            return currentToken;
        }

        if (input[position] >= '0' && input[position] <= '9')
        {
            currentToken.pos = position;

            int i = 0;

            while (input[position] >= '0' && input[position] <= '9')
            {
                currentToken.str[i] = input[position];
                position++;
                i++;
            }

            if (input[position] == '.')
            {
                currentToken.str[i] = input[position];
                position++;
                i++;

                while (input[position] >= '0' && input[position] <= '9')
                {
                    currentToken.str[i] = input[position];
                    position++;
                    i++;
                }

                currentToken.str[i] = '\0';
                currentToken.type = TOKEN_FLOAT;
                currentToken.value.numValue = atof(currentToken.str);
            }
            else
            {
                currentToken.str[i] = '\0';
                currentToken.type = TOKEN_INTEGER;
                currentToken.value.intValue = atol(currentToken.str);
            }

            return currentToken;
        }

        switch (input[position])
        {
            case '+':
                currentToken.type = TOKEN_PLUS;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '-':
                currentToken.type = TOKEN_MINUS;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '*':
                currentToken.type = TOKEN_MULTIPLY;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '/':
                currentToken.type = TOKEN_DIVIDE;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '|':
                currentToken.type = TOKEN_BIT_OR;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '&':
                currentToken.type = TOKEN_BIT_AND;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '^':
                currentToken.type = TOKEN_BIT_XOR;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case '<':
                if (position < inputlen - 1 && input[position + 1] == '<')
                {
                    currentToken.type = TOKEN_LEFT_SHIFT;
                    strncpy(currentToken.str, "<<", sizeof(currentToken.str) - 1);
                    position++;
                }
                else
                {
                    printf("Invalid character: %c, position: %d\n", input[position], position);
                    exit(1);
                }

                break;
            case '>':
                if (position < inputlen - 1 && input[position + 1] == '>')
                {
                    currentToken.type = TOKEN_RIGHT_SHIFT;
                    strncpy(currentToken.str, ">>", sizeof(currentToken.str) - 1);
                    position++;
                }
                else
                {
                    printf("Invalid character: %c, position: %d\n", input[position], position);
                    exit(1);
                }

                break;
            case '(':
                currentToken.type = TOKEN_LPAREN;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            case ')':
                currentToken.type = TOKEN_RPAREN;
                currentToken.str[0] = input[position];
                currentToken.str[1] = '\0';
                break;
            default:
                printf("Invalid character: %c\n", input[position]);
                exit(1);
        }

        position++;
        return currentToken;
    }

    currentToken.type = TOKEN_END;
    return currentToken;
}

double parseExpression()
{
    double result = parseTerm();

    while (currentToken.type == TOKEN_PLUS || currentToken.type == TOKEN_MINUS)
    {
        Token op = currentToken;
        getNextToken();

        double term = parseTerm();

        if (op.type == TOKEN_PLUS)
        {
            result += term;
        }
        else
        {
            result -= term;
        }
    }

    return result;
}

double parseTerm()
{
    double result = parseFactor();

    while (currentToken.type == TOKEN_MULTIPLY || currentToken.type == TOKEN_DIVIDE)
    {
        Token op = currentToken;
        getNextToken();

        double factor = parseFactor();

        if (op.type == TOKEN_MULTIPLY)
        {
            result *= factor;
        }
        else
        {
            result /= factor;
        }
    }

    return result;
}

double parseFactor()
{
    double result = 0;

    if (currentToken.type == TOKEN_INTEGER || currentToken.type == TOKEN_FLOAT)
    {
        result = atof(currentToken.str);
        getNextToken();
    }
    else if (currentToken.type == TOKEN_LPAREN)
    {
        getNextToken();
        result = parseExpression();

        if (currentToken.type != TOKEN_RPAREN)
        {
            printf("Expected ')'\n");
            exit(1);
        }

        getNextToken();
    }
    else
    {
        printf("Expected number or '('\n");
        exit(1);
    }

    return result;
}

void ariMain(void)
{
    input = "(2.5 + 3) * 4.2 - 10.1 / 2 + #1485548 >>  #6545 || 5 ^ 2 & 3<< 16.1235";

    do
    {
        currentToken = getNextTokenMy();
        switch (currentToken.type)
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
                printf("type: TOKEN_OR\t");
                break;
            case TOKEN_BIT_AND:
                printf("type: TOKEN_AND\t");
                break;
            case TOKEN_BIT_XOR:
                printf("type: TOKEN_XOR\t");
                break;
            case TOKEN_LEFT_SHIFT:
                printf("type: TOKEN_LEFT_SHIFT\t");
                break;
            case TOKEN_RIGHT_SHIFT:
                printf("type: TOKEN_RIGHT_SHIFT\t");
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
                printf("type: TOKEN_END\t");
                break;
            default:
                break;
        }

        if (currentToken.type != TOKEN_END)
        {
            printf("\tposition: %u, str: %s", currentToken.pos, currentToken.str);
            if (currentToken.type == TOKEN_INTEGER)
            {
                printf("\tvalue: %d\n", currentToken.value.intValue);
            }
            else if (currentToken.type == TOKEN_FLOAT)
            {
                printf("\tvalue: %f\n", currentToken.value.numValue);
            }
            else
            {
                printf("\n");
            }
        }
    } while (currentToken.type != TOKEN_END);

//    double result = parseExpression();
//
//    printf("Result of the expression: %.2f\n", result);
}
