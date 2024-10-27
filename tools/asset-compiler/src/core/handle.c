#include "core/handle.h"

handle_t handle_create(size_t index) {
    handle_t handle;
    handle.index = index;
    return handle;
}