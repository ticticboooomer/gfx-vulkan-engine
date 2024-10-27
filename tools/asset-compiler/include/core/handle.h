#ifndef CORE_HANDLE_H
#define CORE_HANDLE_H
#include <stddef.h>

typedef struct {
    size_t index;
} handle_t;

handle_t handle_create(size_t index);

#endif