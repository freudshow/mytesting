/*
 *  rdp.c
 * This is a simple implementation of a recursive descent parser
 *  for the grammar:
 *  E -> T E'
 *  E' -> + T E' | ε
 *  T -> F T'
 *  T' -> * F T' | ε
 *  F -> ( E ) | i
 *  Important points about recursive descent parsers
 *  1. Top-Down Parsing: It starts from the start symbol and recursively applies grammar rules to break down the input.
 *  2. One Function per Non-Terminal: Each grammar rule has a corresponding function in the parser, making the implementation straightforward.
 *  3. Uses Recursion: The parser calls functions within themselves to process different parts of the input, matching the recursive nature of grammar rules.
 *  4. Works Best with LL(1) Grammars: It’s most effective for grammars that can be parsed with a single token lookahead, typically simple, non-left-recursive grammars.
 *  5. Easy to Implement: The structure is easy to follow and implement, making it a good choice for small compilers or interpreters.
 *  6. Error Handling: It can detect syntax errors and report them, making it useful for debugging input strings.
 *  Created on: Jul 14, 2025
 *      Author: floyd
 */

#include <stdio.h>
#include <string.h>

#define SUCCESS 1
#define FAILED 0

// Function prototypes
int E(), Edash(), T(), Tdash(), F();

const char *cursor;
char string[64];

int rdpmain()
{
    puts("Enter the string");
    scanf("%s", string); // Read input from the user
    cursor = string;
    puts("");
    puts("Input          Action");
    puts("--------------------------------");

    // Call the starting non-terminal E
    if (E() && *cursor == '\0')
    { // If parsing is successful and the cursor has reached the end
        puts("--------------------------------");
        puts("String is successfully parsed");
        return 0;
    }
    else
    {
        puts("--------------------------------");
        puts("Error in parsing String");
        return 1;
    }
}

// Grammar rule: E -> T E'
int E()
{
    printf("%-16s E -> T E'\n", cursor);

    if (T())
    { // Call non-terminal T
        if (Edash())
        { // Call non-terminal E'
            return SUCCESS;
        }
        else
        {
            return FAILED;
        }
    }
    else
    {
        return FAILED;
    }
}

// Grammar rule: E' -> + T E' | $
int Edash()
{
    if (*cursor == '+')
    {
        printf("%-16s E' -> + T E'\n", cursor);
        cursor++;

        if (T())
        { // Call non-terminal T
            if (Edash())
            { // Call non-terminal E'
                return SUCCESS;
            }
            else
            {
                return FAILED;
            }
        }
        else
        {
            return FAILED;
        }
    }
    else
    {
        printf("%-16s E' -> ε\n", cursor);
        return SUCCESS;
    }
}

// Grammar rule: T -> F T'
int T()
{
    printf("%-16s T -> F T'\n", cursor);

    if (F())
    { // Call non-terminal F
        if (Tdash())
        { // Call non-terminal T'
            return SUCCESS;
        }
        else
        {
            return FAILED;
        }
    }
    else
    {
        return FAILED;
    }
}

// Grammar rule: T' -> * F T' | $
int Tdash()
{
    if (*cursor == '*')
    {
        printf("%-16s T' -> * F T'\n", cursor);
        cursor++;

        if (F())
        { // Call non-terminal F
            if (Tdash())
            { // Call non-terminal T'
                return SUCCESS;
            }
            else
            {
                return FAILED;
            }
        }
        else
        {
            return FAILED;
        }
    }
    else
    {
        printf("%-16s T' -> ε\n", cursor);
        return SUCCESS;
    }
}

// Grammar rule: F -> ( E ) | i
int F()
{
    if (*cursor == '(')
    {
        printf("%-16s F -> ( E )\n", cursor);
        cursor++;

        if (E())
        { // Call non-terminal E
            if (*cursor == ')')
            {
                cursor++;
                return SUCCESS;
            }
            else
            {
                return FAILED;
            }
        }
        else
        {
            return FAILED;
        }
    }
    else if (*cursor == 'i')
    {
        printf("%-16s F -> i\n", cursor);
        cursor++;
        return SUCCESS;
    }
    else
    {
        return FAILED;
    }
}
