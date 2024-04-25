#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token types
typedef enum {
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_REALDB,
    TOKEN_END
} TokenType;

// Token struct
typedef struct {
    TokenType type;
    char *value;
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

        if (input[position] == '#')
        {
            char *value = malloc(sizeof(char) * 100);
            int i = 0;
            while (input[position] >= '0' && input[position] <= '9')
            {
                value[i] = input[position];
                position++;
                i++;
            }

            value[i] = '\0';
            currentToken.type = TOKEN_REALDB;
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

            currentToken.value = strdup(value);
            return currentToken;
        }

        switch (input[position])
        {
            case '+':
                currentToken.type = TOKEN_PLUS;
                currentToken.value = calloc(4, sizeof(char));
                currentToken.value[0] = input[position];
                break;
            case '-':
                currentToken.type = TOKEN_MINUS;
                currentToken.value = calloc(4, sizeof(char));
                currentToken.value[0] = input[position];
                break;
            case '*':
                currentToken.type = TOKEN_MULTIPLY;
                currentToken.value = calloc(4, sizeof(char));
                currentToken.value[0] = input[position];
                break;
            case '/':
                currentToken.type = TOKEN_DIVIDE;
                currentToken.value = calloc(4, sizeof(char));
                currentToken.value[0] = input[position];
                break;
            case '(':
                currentToken.type = TOKEN_LPAREN;
                currentToken.value = calloc(4, sizeof(char));
                currentToken.value[0] = input[position];
                break;
            case ')':
                currentToken.type = TOKEN_RPAREN;
                currentToken.value = calloc(4, sizeof(char));
                currentToken.value[0] = input[position];
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
        result = atof(currentToken.value);
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
    input = "(2.5 + 3) * 4.2 - 10.1 / 2";

    do
    {
        currentToken = getNextToken();
        switch (currentToken.type)
        {
            case TOKEN_INTEGER:
                printf("type: TOKEN_INTEGER\n");
                break;
            case TOKEN_FLOAT:
                printf("type: TOKEN_FLOAT\n");
                break;
            case TOKEN_PLUS:
                printf("type: TOKEN_PLUS\n");
                break;
            case TOKEN_MINUS:
                printf("type: TOKEN_MINUS\n");
                break;
            case TOKEN_MULTIPLY:
                printf("type: TOKEN_MULTIPLY\n");
                break;
            case TOKEN_DIVIDE:
                printf("type: TOKEN_DIVIDE\n");
                break;
            case TOKEN_LPAREN:
                printf("type: TOKEN_LPAREN\n");
                break;
            case TOKEN_RPAREN:
                printf("type: TOKEN_RPAREN\n");
                break;
            case TOKEN_REALDB:
                printf("type: TOKEN_REALDB\n");
                break;
            case TOKEN_END:
                printf("type: TOKEN_END\n");
                break;
            default:
                break;
        }

        if (currentToken.type != TOKEN_END)
        {
            printf("value: %s\n", currentToken.value);
        }

        currentToken = getNextToken();
    } while (currentToken.type != TOKEN_END);

//    double result = parseExpression();
//
//    printf("Result of the expression: %.2f\n", result);
}
