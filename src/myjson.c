#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#define MAX_TOKEN_LENGTH 1024
#define MAX_OBJECT_DEPTH 16

typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_OBJECT,
    JSON_ARRAY
} json_type;

// Token structure to represent a parsed JSON token (e.g., string, object key)
typedef struct {
    char* value;
    size_t length;
} json_token;

// Function to parse the next token from the input file
json_token* parse_token(const char* input) {
    // Find the next whitespace character or the end of the line
    const char* start = input;
    while (*input != '\0' && *input <= ' ') {
        input++;
    }

    // Check if it's a quoted string (or null)
    if (*input == '"' || *input == '\'') {
        size_t length = 0;
        while (*input++ != *start && *input != '\0') {
            length++;
        }
        json_token* token = malloc(sizeof(json_token));
        token->value = malloc(length + 1);
        strncpy(token->value, start, length);
        token->value[length] = '\0';
        token->length = length;
        return token;
    }

    // Check if it's a keyword (e.g., "null", "true", etc.)
    static const char* keywords[] = {"null", "true", "false"};
    for (size_t i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
        size_t length = strlen(keywords[i]);
        if (!strncmp(input, keywords[i], length)) {
            json_token* token = malloc(sizeof(json_token));
            token->value = strdup(keywords[i]);
            token->length = length;
            return token;
        }
    }

    // If none of the above, assume it's an object key
    size_t length = 0;
    while (*input++ != '{' && *input != '\0') {
        length++;
    }
    json_token* token = malloc(sizeof(json_token));
    token->value = malloc(length + 1);
    strncpy(token->value, start, length);
    token->value[length] = '\0';
    token->length = length;
    return token;
}

// Function to parse the next object key-value pair
json_token* parse_object(const char* input) {
    // Parse the key
    json_token* key_token = parse_token(input);

    // Skip over the colon and whitespace
    while (*input <= ' ') {
        input++;
    }

    // Parse the value
    if (*input == '{') {  // Object
        size_t length = 0;
        while (*input++ != '}') {
            length++;
        }
        json_token* token = malloc(sizeof(json_token));
        token->value = malloc(length + 1);
        strncpy(token->value, input - length, length);
        token->value[length] = '\0';
        token->length = length;
        return token;
    } else {  // String
        size_t length = 0;
        while (*input++ != '"') {
            length++;
        }
        json_token* token = malloc(sizeof(json_token));
        token->value = malloc(length + 1);
        strncpy(token->value, input - length, length);
        token->value[length] = '\0';
        token->length = length;
        return token;
    }
}

// Function to parse the entire JSON file
json_token* parse_json(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        // Handle error
        return NULL;
    }

    json_token* current_token = NULL;
    size_t depth = 0;

    while (1) {
        const char* line = NULL;
        size_t length = 0;
        while ((line = fgets(file, MAX_TOKEN_LENGTH)) != NULL && *line <= ' ') {
            // Skip over whitespace
        }

        if (!line) {
            break;  // End of file
        }

        if (*line == '{') {  // Start of an object
            depth++;
            current_token = parse_object(line + 1);
        } else if (*line == '}') {  // End of an object
            depth--;
            if (depth == 0) {
                break;  // End of file
            }
        } else {  // Token or whitespace
            size_t length = 0;
            while (*line++ != '\0' && *line <= ' ') {
                length++;
            }
            json_token* token = malloc(sizeof(json_token));
            token->value = malloc(length + 1);
            strncpy(token->value, line - length, length);
            token->value[length] = '\0';
            token->length = length;
            current_token = token;
        }
    }

    fclose(file);

    return current_token;
}

int myjsonmain(int argc, char* argv[]) {
    const char* filename = "example.json";
    json_token* token = parse_json(filename);

    // Print the parsed JSON token
    printf("%s\n", token->value);
    free(token);

    return 0;
}
