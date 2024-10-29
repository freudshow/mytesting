#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the structure for a key-value pair
typedef struct Pair {
    char *key;
    int value;
    struct Pair *next;
} Pair;

// Define the structure for the hash table
typedef struct HashTable {
    int size;
    Pair **buckets;
} HashTable;

// Function to create a new hash table
HashTable* createHashTable(int size)
{
    HashTable *table = (HashTable*) malloc(sizeof(HashTable));
    table->size = size;
    table->buckets = (Pair**) calloc(size, sizeof(Pair*));
    return table;
}

// Function to hash a key
int hash(char *key, int size)
{
    int hashValue = 0;
    for (int i = 0; i < strlen(key); i++)
    {
        hashValue += key[i];
    }
    return hashValue % size;
}

// Function to insert a key-value pair into the hash table
void insert(HashTable *table, char *key, int value)
{
    int index = hash(key, table->size);
    Pair *pair = (Pair*) malloc(sizeof(Pair));
    pair->key = (char*) malloc(strlen(key) + 1);
    strcpy(pair->key, key);
    pair->value = value;
    pair->next = table->buckets[index];
    table->buckets[index] = pair;
}

// Function to retrieve the value associated with a key
int get(HashTable *table, char *key)
{
    int index = hash(key, table->size);
    Pair *pair = table->buckets[index];
    while (pair != NULL)
    {
        if (strcmp(pair->key, key) == 0)
        {
            return pair->value;
        }
        pair = pair->next;
    }
    return -1; // Key not found
}

// Function to update the value associated with a key
void update(HashTable *table, char *key, int value)
{
    int index = hash(key, table->size);
    Pair *pair = table->buckets[index];
    while (pair != NULL)
    {
        if (strcmp(pair->key, key) == 0)
        {
            pair->value = value;
            return;
        }
        pair = pair->next;
    }
    // Key not found, insert a new key-value pair
    insert(table, key, value);
}

// Function to delete a key-value pair from the hash table
void delete(HashTable *table, char *key)
{
    int index = hash(key, table->size);
    Pair *pair = table->buckets[index];
    Pair *prev = NULL;
    while (pair != NULL)
    {
        if (strcmp(pair->key, key) == 0)
        {
            if (prev == NULL)
            {
                table->buckets[index] = pair->next;
            }
            else
            {
                prev->next = pair->next;
            }
            free(pair->key);
            free(pair);
            return;
        }
        prev = pair;
        pair = pair->next;
    }
}

// Function to print the contents of the hash table
void printHashTable(HashTable *table)
{
    for (int i = 0; i < table->size; i++)
    {
        Pair *pair = table->buckets[i];
        printf("Bucket %d:\n", i);
        while (pair != NULL)
        {
            printf("  Key: %s, Value: %d\n", pair->key, pair->value);
            pair = pair->next;
        }
    }
}

void testhash(void)
{
    HashTable *table = createHashTable(10);
    insert(table, "apple", 5);
    insert(table, "banana", 7);
    insert(table, "orange", 3);
    printHashTable(table);
    printf("Value of apple: %d\n", get(table, "apple"));
    update(table, "apple", 10);
    printf("Value of apple after update: %d\n", get(table, "apple"));
    delete(table, "banana");
    printHashTable(table);
}
