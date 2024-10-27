#ifndef BACKEND_GPU_H
#define BACKEND_GPU_H

#include <stdint.h>
#include <vulkan/vulkan_core.h>

void gpu_init_vk(const char* app_name, uint32_t app_version);
VkInstance gpu_get_vk_instance();
VkDevice gpu_get_vk_device();
VkPhysicalDevice gpu_get_vk_phy_device();
VkSwapchainKHR gpu_get_vk_swapchain();
VkExtent2D gpu_get_swapchain_extent();
VkFormat gpu_get_swapchain_format();
VkRenderPass gpu_get_render_pass();
void gpu_destroy_vk();

#endif