#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H

#include <stdlib.h>

typedef struct Range {
    size_t start;
    size_t end;
} Range;

typedef struct HashTableItem {
    int isBusy;
    Range key;
    Range value;
} HashTableItem;

typedef struct HashTable {
    char* keysBuffer;
    size_t keysBufferLength;

    HashTableItem* valueBuffer;
    size_t valueBufferLength;
} HashTable;

int HashTable_set(HashTable* table, size_t keyStart, size_t keyEnd, size_t valueStart, size_t valueEnd);
Range* HashTable_get(HashTable* table, char* key);

#endif // HASH_TABLE_H
