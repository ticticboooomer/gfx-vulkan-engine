#include "core/bitwise.h"

u32 bitmask_flags_set_on(u32 mask, u32 bit_index) {
    return mask | 1 << bit_index;
}

u32 bitmask_flags_set_off(u32 mask, u32 bit_index) {
    return mask & ~(1 << bit_index);
}

u32 bitmask_flags_get(u32 mask, u32 bit_index) {
    return mask & (1 << bit_index);
}
