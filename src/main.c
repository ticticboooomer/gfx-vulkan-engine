#include <stdio.h>
#include <vulkan/vulkan.h>
#include "backend/gpu.h"


int main(int argc, const char **argv) {
  printf("sosig a");
  ptia_init_vk("game A", VK_MAKE_VERSION(0, 0, 1));

  printf("sosig b");
  ptia_destroy_vk();
}
