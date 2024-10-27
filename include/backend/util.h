#ifndef BACKEND_UTIL_H
#define BACKEND_UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t clamp(uint32_t i, uint32_t min, uint32_t max);

typedef struct {
    char *data;
    size_t size;
} file_str_t;

file_str_t read_file(const char *fname);

#endif
