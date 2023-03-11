#include "HashTable.h"

#include <stdio.h>
#include <string.h>

static unsigned long hashChars(char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static unsigned long hashRange(char* keysBuffer, Range* range) {
    unsigned long hash = 5381;
    int c;

    for(size_t i = range->start; i <= range->end; i++) {
        c = keysBuffer[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
        
    return hash;
}

int HashTable_set(HashTable* table, size_t keyStart, size_t keyEnd, size_t valueStart, size_t valueEnd) {
    Range range = {
        .start = keyStart,
        .end = keyEnd,
    };
    unsigned long hash = hashRange(table->keysBuffer, &range);

    size_t index = hash % table->valueBufferSize;

    HashTableItem* item = &table->valueBuffer[index];

    item->isBusy = 1;
    item->key.start = keyStart;
    item->key.end = keyEnd;

    item->value.start = valueStart;
    item->value.end = valueEnd;

    table->values++;

    return 0;
}

Range* HashTable_get(HashTable* table, char* key) {
    unsigned long hash = hashChars(key);

    size_t index = hash % table->valueBufferSize;

    if(!table->valueBuffer[index].isBusy) return NULL;

    return &table->valueBuffer[index].value;
}

void HashTable_clear(HashTable* table) {
    table->values = 0;
    memset(table->valueBuffer, 0, table->valueBufferSize * sizeof(HashTableItem));
}

void HashTable_initIterator(HashTable* table, Iterator* iterator) {
    iterator->table = table;
    iterator->index = 0;
}

HashTableItem* Iterator_next(Iterator* iterator) {
    while(iterator->index < iterator->table->valueBufferSize) {
        if(iterator->table->valueBuffer[iterator->index].isBusy) {
            iterator->index++;
            return &iterator->table->valueBuffer[iterator->index - 1];
        } else {
            iterator->index++;
        }
    }

    return NULL;
}