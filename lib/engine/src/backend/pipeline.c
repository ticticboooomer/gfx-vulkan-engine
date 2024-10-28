#include "backend/pipeline.h"
#include "backend/gpu.h"
#include "backend/util.h"
#include "error.h"
#include <vulkan/vulkan_core.h>

VkShaderModule _create_shader_module(file_str_t code) {
  VkShaderModuleCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size;
  create_info.pCode = (const uint32_t *)code.data;
  create_info.flags = 0;
  create_info.pNext = 0;

  VkShaderModule module;
  if (vkCreateShaderModule(gpu_get_vk_device(), &create_info, 0, &module) !=
      VK_SUCCESS) {
    ptia_panic("Failed to create vk Shader Module");
  }
  return module;
}

VkPipelineShaderStageCreateInfo
_create_shader_stage(VkShaderStageFlagBits stage, VkShaderModule module,
                     const char *pName) {
  VkPipelineShaderStageCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  create_info.stage = stage;
  create_info.module = module;
  create_info.pName = pName;
  create_info.pNext = 0;
  create_info.flags = 0;
  return create_info;
}

void _init_vk_graphics_pipeline(pipeline_def_t* def) {
  file_str_t vert_shader_code = read_file("shaders/vert.spv");
  file_str_t frag_shader_code = read_file("shaders/frag.spv");

  VkShaderModule vert_shader = _create_shader_module(vert_shader_code);
  VkShaderModule frag_shader = _create_shader_module(frag_shader_code);

  def->shaders = malloc(sizeof(VkShaderModule) * 2);
  def->shader_count = 2;
  def->shaders[0] = vert_shader;
  def->shaders[1] = frag_shader;

  VkPipelineShaderStageCreateInfo stages[] = {
      _create_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, vert_shader,
                           "main"),
      _create_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader,
                           "main")};

  VkDynamicState dyn_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                 VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dyn_states_create_info;
  dyn_states_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dyn_states_create_info.dynamicStateCount = 2;
  dyn_states_create_info.pDynamicStates = dyn_states;
  dyn_states_create_info.pNext = 0;
  dyn_states_create_info.flags = 0;



  VkPipelineVertexInputStateCreateInfo vtx_input_create_info;
  vtx_input_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vtx_input_create_info.vertexBindingDescriptionCount = 0;
  vtx_input_create_info.pVertexBindingDescriptions = 0;
  vtx_input_create_info.vertexAttributeDescriptionCount = 0;
  vtx_input_create_info.pVertexAttributeDescriptions = 0;
  vtx_input_create_info.pNext = 0;
  vtx_input_create_info.flags = 0;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info;
  input_assembly_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
  input_assembly_create_info.pNext = 0;
  input_assembly_create_info.flags = 0;

  VkExtent2D swapchain_extent = gpu_get_swapchain_extent();

  VkViewport viewport;
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = swapchain_extent.width;
  viewport.height = swapchain_extent.height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = swapchain_extent;

  VkPipelineViewportStateCreateInfo vport_create_info;
  vport_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vport_create_info.viewportCount = 1;
  vport_create_info.scissorCount = 1;
  vport_create_info.pScissors = &scissor;
  vport_create_info.pViewports = &viewport;
  vport_create_info.pNext = 0;
  vport_create_info.flags = 0;

  VkPipelineRasterizationStateCreateInfo rasterizer;
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.f;
  rasterizer.depthBiasClamp = 0.f;
  rasterizer.depthBiasSlopeFactor = 0.f;
  rasterizer.pNext = 0;
  rasterizer.flags = 0;

  VkPipelineMultisampleStateCreateInfo multisample_create_info;
  multisample_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_create_info.sampleShadingEnable = VK_FALSE;
  multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_create_info.minSampleShading = 1.0f;
  multisample_create_info.pSampleMask = 0;
  multisample_create_info.alphaToCoverageEnable = VK_FALSE;
  multisample_create_info.alphaToOneEnable = VK_FALSE;
  multisample_create_info.pNext = 0;
  multisample_create_info.flags = 0;

  VkPipelineColorBlendAttachmentState color_blend_attachment;
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo color_blending;
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;
  color_blending.flags = 0;
  color_blending.pNext = 0;

  VkPipelineLayoutCreateInfo pipeline_layout_create_info;
  pipeline_layout_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = 0;
  pipeline_layout_create_info.pSetLayouts = 0;
  pipeline_layout_create_info.pPushConstantRanges = 0;
  pipeline_layout_create_info.pushConstantRangeCount = 0;
  pipeline_layout_create_info.flags = 0;
  pipeline_layout_create_info.pNext = 0;

  if (vkCreatePipelineLayout(gpu_get_vk_device(), &pipeline_layout_create_info, 0,
                             &def->layout) != VK_SUCCESS) {
    ptia_panic("Failed to create vk Pipeline Layout");
  }
  VkGraphicsPipelineCreateInfo pipeline_create_info;
  pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info.pNext = 0;
  pipeline_create_info.flags = 0;
  pipeline_create_info.stageCount = 2;
  pipeline_create_info.pStages = stages;
  pipeline_create_info.pVertexInputState = &vtx_input_create_info;
  pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
  pipeline_create_info.pViewportState = &vport_create_info;
  pipeline_create_info.pRasterizationState = &rasterizer;
  pipeline_create_info.pMultisampleState = &multisample_create_info;
  pipeline_create_info.pDepthStencilState = 0;
  pipeline_create_info.pColorBlendState = &color_blending;
  pipeline_create_info.pDynamicState = &dyn_states_create_info;
  pipeline_create_info.layout = def->layout;
  pipeline_create_info.renderPass = gpu_get_render_pass();
  pipeline_create_info.subpass = 0;
  pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_create_info.basePipelineIndex = -1;
  pipeline_create_info.pTessellationState = 0;

  if (vkCreateGraphicsPipelines(gpu_get_vk_device(), VK_NULL_HANDLE, 1,
                                &pipeline_create_info, 0,
                                &def->pipeline) != VK_SUCCESS) {
    ptia_panic("Failed to create graphics pipeline");
  }
}

void create_command_pool() {

}

pipeline_def_t create_graphics_pipeline() {
  pipeline_def_t def;
  _init_vk_graphics_pipeline(&def);
  return def;
}

void destroy_graphics_pipeline(pipeline_def_t* def) {
  VkDevice device = gpu_get_vk_device();


  vkDestroyPipeline(device, def->pipeline, 0);
  vkDestroyPipelineLayout(device, def->layout, 0);
  vkDestroyShaderModule(device, def->shaders[1], 0);
  vkDestroyShaderModule(device, def->shaders[0], 0);

}
