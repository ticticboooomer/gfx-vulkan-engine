#pragma once
#include <vulkan/vulkan_core.h>

typedef struct {
  VkPipelineLayout layout;
  VkPipeline pipeline;
  VkShaderModule* shaders;
  uint32_t shader_count;
} pipeline_def_t;

pipeline_def_t create_graphics_pipeline();
