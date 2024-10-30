#ifndef CORE_HANDLE_H
#define CORE_HANDLE_H

#include "defines.h"

typedef struct {
    u64 index;
} handle_t;

handle_t handle_create(u64 index);

#endif