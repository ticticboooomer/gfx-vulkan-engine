#include "core/handle.h"

handle_t handle_create(u64 index) {
    handle_t handle;
    handle.index = index;
    return handle;
}