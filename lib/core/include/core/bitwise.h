#ifndef CORE_BIT_WISE_H
#define CORE_BIT_WISE_H

#include "core/defines.h"

u32 bitmask_flags_set_on(u32 mask, u32 bit_index);
u32 bitmask_flags_set_off(u32 mask, u32 bit_index);
u32 bitmask_flags_get(u32 mask, u32 bit_index);

#endif