#include <stdint.h>
#include <vulkan/vulkan_core.h>

void ptia_init_vk(const char* app_name, uint32_t app_version);
VkInstance ptia_get_vk_instance();

void ptia_destroy_vk();
