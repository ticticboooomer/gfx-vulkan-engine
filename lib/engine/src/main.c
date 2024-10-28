#include <vulkan/vulkan.h>
#include "backend/gpu.h"
#include "backend/pipeline.h"
#include "backend/window.h"

int main(int argc, const char **argv) {
  window_preset_resolution(1000, 800);
  window_set_title("sosig game");
  gpu_init_vk("game A", VK_MAKE_VERSION(0, 0, 1));
  pipeline_def_t gfx_pipeline = create_graphics_pipeline();
  gpu_destroy_vk();
}