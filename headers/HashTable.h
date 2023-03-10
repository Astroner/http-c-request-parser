#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H

#include "main.h"

#include <stdlib.h>

typedef struct HashTableItem {
    int isBusy;
    Range key;
    Range value;
} HashTableItem;

typedef struct HashTable {
    char* keysBuffer;
    size_t keysBufferLength;
    size_t values;

    HashTableItem* valueBuffer;
    size_t valueBufferSize;
} HashTable;

typedef struct Iterator {
    HashTable* table;
    size_t index;
} Iterator;

int HashTable_set(HashTable* table, size_t keyStart, size_t keyEnd, size_t valueStart, size_t valueEnd);
Range* HashTable_get(HashTable* table, char* key);
void HashTable_initIterator(HashTable* table, Iterator* iterator);
void HashTable_clear(HashTable* table);

HashTableItem* Iterator_next(Iterator*);


#endif // HASH_TABLE_H
