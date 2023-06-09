#if !defined(MAIN_H)
#define MAIN_H

#include <stdlib.h>

typedef struct Range {
    size_t start;
    size_t end;
} Range;

char* extractRange(char* buffer, size_t start, size_t end);

typedef enum PrintRangeMode {
    PrintRangeModeDefault,
    PrintRangeModeWithCodes,
    PrintRangeModeCodesOnly,
} PrintRangeMode;

void printRange(char* buffer, size_t start, size_t end, PrintRangeMode mode);

#endif // MAIN_H
