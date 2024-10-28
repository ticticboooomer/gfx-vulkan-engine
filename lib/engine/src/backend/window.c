#include "engine/backend/window.h"

static uint32_t g_window_width = 0;
static uint32_t g_window_height = 0;
static char* g_window_title = "";

void window_preset_resolution(uint32_t w, uint32_t h) {
  g_window_width = w;
  g_window_height = h;
}

void window_get_resolution(uint32_t *pW, uint32_t *pH) {
  *pW = g_window_width;
  *pH = g_window_height;
}

void window_set_title(char *title) {
  g_window_title = title;
}

char* window_get_title() {
  return g_window_title;
}
