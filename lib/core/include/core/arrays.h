#ifndef CORE_ARRAYS_H
#define CORE_ARRAYS_H


#include <stdlib.h>
#include "defines.h"

#define OVERALLOC 8

void ensure_capacity_overalloc(void** parray, u64* arr_size, u64 ensure_size, u64 elem_size) {
    if (*arr_size < ensure_size) {
        u64 new_len = (ensure_size + OVERALLOC);
        *parray = realloc(*parray, new_len * elem_size);
        *arr_size = new_len;
    }
}

#endif