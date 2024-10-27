
#ifndef BACKEND_WINDOW_H
#define BACKEND_WINDOW_H

#include <stdint.h>
void window_preset_resolution(uint32_t w, uint32_t h);
void window_get_resolution(uint32_t* pW, uint32_t* pH);
void window_set_title(char* title);
char* window_get_title();

#endif