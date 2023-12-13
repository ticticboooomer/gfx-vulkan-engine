#include "backend/gpu.h"
#include "GLFW/glfw3.h"
#include "backend/util.h"
#include "error.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define NDEBUG 1

typedef struct {
  uint32_t count;
  VkExtensionProperties *exts;
} device_extensions_t;

typedef struct {
  uint32_t gfx_family;
  uint32_t present_family;
} queue_family_indices_t;

typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  VkPresentModeKHR *present_modes;
  uint32_t format_count;
  uint32_t present_mode_count;
} swap_chain_support_details_t;

static VkInstance g_vk_instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_vk_physical_device = VK_NULL_HANDLE;
static VkDevice g_vk_device = VK_NULL_HANDLE;
static VkQueue g_vk_gfx_queue = VK_NULL_HANDLE;
static VkQueue g_vk_present_queue = VK_NULL_HANDLE;
static VkSurfaceKHR g_vk_surface = VK_NULL_HANDLE;
static VkSwapchainKHR g_vk_swapchain = VK_NULL_HANDLE;

// 0: refactor for isntancing
static VkPipelineLayout g_vk_pipeline_layout = VK_NULL_HANDLE;

static VkShaderModule g_vk_frag_shader = VK_NULL_HANDLE;
static VkShaderModule g_vk_vert_shader = VK_NULL_HANDLE;
// 0: end
static GLFWwindow *g_wnd = 0;

static VkFormat g_vk_swapchain_format;
static VkExtent2D g_vk_swapchain_extent;

static VkImage *g_vk_swapchain_images = 0;
static uint32_t g_vk_swapchain_image_count = 0;

static VkImageView *g_vk_image_views = 0;
static uint32_t g_vk_image_view_count = 0;

static char *g_validation_layers[1] = {"VK_LAYER_KHRONOS_validation"};
static uint32_t g_validation_layers_count = 1;

static char *g_device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static uint32_t g_device_extensions_count = 1;

static VkDebugUtilsMessengerEXT g_debug_messenger = VK_NULL_HANDLE;
#ifdef NDEBUG
const int USE_VALIDATION_LAYERS = 1;
#else
const int USE_VALIDATION_LAYERS = 0;
#endif /* ifdef MACRO */

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  fprintf(stderr, "validation layer: %s", pCallbackData->pMessage);

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pCallback) {
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
  instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != 0) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT callback,
                                   const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != 0) {
    func(instance, callback, pAllocator);
  }
}

void setup_debug_messenger() {
  VkDebugUtilsMessengerCreateInfoEXT create_info;
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debugCallback;
  create_info.pUserData = 0;
  create_info.pNext = 0;
  create_info.flags = 0;

  VkResult tmp = CreateDebugUtilsMessengerEXT(g_vk_instance, &create_info, 0,
                                              &g_debug_messenger);
  if (tmp != VK_SUCCESS) {
    ptia_panic("Failed to create Debug messenger");
  }
}

int check_validation_layer_support() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, 0);

  VkLayerProperties *availableLayers =
      malloc(sizeof(VkLayerProperties) * layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

  for (int i = 0; i < g_validation_layers_count; i++) {
    int layerFound = 0;
    char *layerName = g_validation_layers[i];

    for (int j = 0; j < layerCount; j++) {
      VkLayerProperties layerProps = availableLayers[j];

      if (strcmp(layerName, layerProps.layerName) == 0) {
        layerFound = 1;
        break;
      }
    }

    if (layerFound != 1) {
      return 0;
    }
  }
  return 1;
}

void _init_vk_instance(const char *app_name, uint32_t app_version) {
  if (USE_VALIDATION_LAYERS && !check_validation_layer_support()) {
    ptia_panic("Application built for validation layer usage, however, no "
               "support found");
  }
  VkApplicationInfo app_info;

  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_API_VERSION_1_3;
  app_info.pEngineName = "Potentia Engine";
  app_info.pApplicationName = "Game Thingy";
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pNext = 0;

  VkInstanceCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.pNext = 0;
  create_info.flags = 0;

  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  char **actual_exts = malloc(sizeof(char *) * glfw_extension_count + 1);
  actual_exts[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  for (int i = 0; i < glfw_extension_count; i++) {
    actual_exts[i + 1] = (char *)glfw_extensions[i];
  }
  create_info.enabledExtensionCount = glfw_extension_count + 1;
  create_info.ppEnabledExtensionNames = (const char **)actual_exts;
  if (USE_VALIDATION_LAYERS) {
    create_info.enabledLayerCount = g_validation_layers_count;
    create_info.ppEnabledLayerNames = (const char *const *)g_validation_layers;
  } else {
    create_info.enabledLayerCount = 0;
  }

  if (vkCreateInstance(&create_info, 0, &g_vk_instance) != VK_SUCCESS) {
    ptia_panic("Failed to create vk Instance");
  }
  if (USE_VALIDATION_LAYERS) {
    setup_debug_messenger();
  }
}

queue_family_indices_t _find_queue_families(VkPhysicalDevice device) {
  queue_family_indices_t indices;
  indices.gfx_family = ~(0u);
  indices.present_family = ~(0u);

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);

  VkQueueFamilyProperties *queue_families =
      malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);
  for (uint32_t i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.gfx_family = i;
    }

    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_vk_surface,
                                         &present_support);
    if (present_support) {
      indices.present_family = i;
    }
  }
  return indices;
}

uint8_t _match_required_device_exts(VkExtensionProperties *available,
                                    uint32_t count) {
  for (uint32_t av = 0; av < g_device_extensions_count; av++) {
    uint8_t found = 0;
    for (uint32_t i = 0; i < count; i++) {
      if (strcmp(available[i].extensionName, g_device_extensions[av])) {
        found = 1;
      }
    }
    if (found == 0) {
      return 0;
    }
  }
  return 1;
}

device_extensions_t _get_device_exts(VkPhysicalDevice device) {
  uint32_t ext_count = 0;
  vkEnumerateDeviceExtensionProperties(device, 0, &ext_count, 0);

  VkExtensionProperties *available_extensions =
      malloc(sizeof(VkExtensionProperties) * ext_count);
  vkEnumerateDeviceExtensionProperties(device, 0, &ext_count,
                                       available_extensions);
  device_extensions_t result;
  result.exts = available_extensions;
  result.count = ext_count;
  return result;
}

char **_get_vk_ext_names(device_extensions_t extensions) {
  char **result = malloc(sizeof(char *) * extensions.count);
  for (uint32_t i = 0; i < extensions.count; i++) {
    result[i] = extensions.exts[i].extensionName;
  }
  return result;
}

void _init_vk_logical_device() {
  queue_family_indices_t indices = _find_queue_families(g_vk_physical_device);

  uint32_t unique_indices_count = 2;
  VkDeviceQueueCreateInfo *queue_create_infos =
      malloc(sizeof(VkDeviceQueueCreateInfo) * unique_indices_count);
  uint32_t unique_indices[2] = {indices.gfx_family, indices.present_family};
  float queue_priority = 1.f;

  for (uint32_t i = 0; i < unique_indices_count; i++) {
    VkDeviceQueueCreateInfo queue_create_info;
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = unique_indices[i];
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_info.pNext = 0;
    queue_create_info.flags = 0;
    queue_create_infos[i] = queue_create_info;
  }

  VkPhysicalDeviceFeatures device_features;
  vkGetPhysicalDeviceFeatures(g_vk_physical_device, &device_features);

  device_extensions_t exts = _get_device_exts(g_vk_physical_device);
  char **ext_names = _get_vk_ext_names(exts);

  VkDeviceCreateInfo device_create_info;
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = queue_create_infos;
  device_create_info.queueCreateInfoCount = unique_indices_count;
  device_create_info.pEnabledFeatures = &device_features;
  device_create_info.enabledExtensionCount = g_device_extensions_count;
  device_create_info.ppEnabledExtensionNames = g_device_extensions;
  device_create_info.flags = 0;
  device_create_info.pNext = 0;

  if (USE_VALIDATION_LAYERS) {
    device_create_info.enabledLayerCount = g_validation_layers_count;
    device_create_info.ppEnabledLayerNames = (const char **)g_validation_layers;
  } else {
    device_create_info.enabledLayerCount = 0;
  }

  VkResult res = vkCreateDevice(g_vk_physical_device, &device_create_info, 0,
                                &g_vk_device);
  if (res != VK_SUCCESS) {
    ptia_panic("Failed to create logical vk device");
  }
  vkGetDeviceQueue(g_vk_device, indices.gfx_family, 0, &g_vk_gfx_queue);
  vkGetDeviceQueue(g_vk_device, indices.present_family, 0, &g_vk_present_queue);
}

uint8_t _check_device_ext_support(VkPhysicalDevice device) {
  device_extensions_t exts = _get_device_exts(device);

  return _match_required_device_exts(exts.exts, exts.count);
}

swap_chain_support_details_t
_query_swap_chain_support(VkPhysicalDevice device) {
  swap_chain_support_details_t details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_vk_surface,
                                            &details.capabilities);
  details.format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_vk_surface,
                                       &details.format_count, 0);
  if (details.format_count != 0) {
    details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, g_vk_surface, &details.format_count, details.formats);
  }
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vk_surface,
                                            &details.present_mode_count, 0);
  if (details.present_mode_count != 0) {
    details.present_modes =
        malloc(sizeof(VkPresentModeKHR) * details.present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_vk_surface,
                                              &details.present_mode_count,
                                              details.present_modes);
  }
  return details;
}

VkSurfaceFormatKHR
_select_swap_chain_format(VkSurfaceFormatKHR *available_formats,
                          uint32_t format_count) {
  for (uint32_t i = 0; i < format_count; i++) {
    VkSurfaceFormatKHR curr_format = available_formats[i];
    if (curr_format.format == VK_FORMAT_R8G8B8A8_SRGB &&
        curr_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return curr_format;
    }
  }
  return available_formats[0];
}

VkPresentModeKHR _select_swap_present_mode(VkPresentModeKHR *available_modes,
                                           uint32_t mode_count) {
  for (uint32_t i = 0; i < mode_count; i++) {
    VkPresentModeKHR curr_mode = available_modes[i];
    if (curr_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return curr_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D _select_swap_extent(VkSurfaceCapabilitiesKHR caps) {
  if (caps.currentExtent.width != ~(0u)) {
    return caps.currentExtent;
  }
  int w, h;
  glfwGetFramebufferSize(g_wnd, &w, &h);
  VkExtent2D actualExtent;
  actualExtent.width = w;
  actualExtent.height = h;

  actualExtent.width = clamp(actualExtent.width, caps.minImageExtent.width,
                             caps.maxImageExtent.width);
  actualExtent.height = clamp(actualExtent.height, caps.minImageExtent.height,
                              caps.maxImageExtent.height);
  return actualExtent;
}

uint8_t _is_device_suitable(VkPhysicalDevice device) {
  queue_family_indices_t indices = _find_queue_families(device);

  uint8_t indices_correct =
      indices.gfx_family != ~(0u) && indices.present_family != ~(0u);

  uint8_t extensions_supported = _check_device_ext_support(device);

  uint8_t swap_chain_suitable = 0;
  if (extensions_supported) {
    swap_chain_support_details_t details = _query_swap_chain_support(device);
    swap_chain_suitable =
        details.present_mode_count > 0 && details.format_count > 0;
  }

  return indices_correct && extensions_supported && swap_chain_suitable;
}

void _init_vk_pick_phy_dev() {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(g_vk_instance, &device_count, 0);

  if (device_count == 0) {
    ptia_panic("No Physical Devices Found");
  }

  VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
  vkEnumeratePhysicalDevices(g_vk_instance, &device_count, devices);

  for (uint32_t i = 0; i < device_count; i++) {
    if (_is_device_suitable(devices[i])) {
      g_vk_physical_device = devices[i];
      break;
    }
  }

  if (g_vk_physical_device == VK_NULL_HANDLE) {
    ptia_panic("Physical device not selected or iniitialised");
  }
}
void _init_glfw() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  //  TODO: un-hardcode this shit
  g_wnd = glfwCreateWindow(1000, 1000, "game A", 0, 0);
}
void _preinit_vk_surface() {
  if (glfwCreateWindowSurface(g_vk_instance, g_wnd, 0, &g_vk_surface) !=
      VK_SUCCESS) {
    ptia_panic("GLFW failed to get surface khr");
  }
  assert(g_vk_surface != VK_NULL_HANDLE);
}

void _create_swapchain() {
  swap_chain_support_details_t sc_details =
      _query_swap_chain_support(g_vk_physical_device);

  VkSurfaceFormatKHR surface_format =
      _select_swap_chain_format(sc_details.formats, sc_details.format_count);
  VkPresentModeKHR surface_present_mode = _select_swap_present_mode(
      sc_details.present_modes, sc_details.present_mode_count);
  VkExtent2D surface_extent = _select_swap_extent(sc_details.capabilities);
  uint32_t imageCount = sc_details.capabilities.minImageCount + 1;
  if (sc_details.capabilities.maxImageCount > 0 &&
      imageCount > sc_details.capabilities.maxImageCount) {
    imageCount = sc_details.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info;
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = g_vk_surface;
  create_info.minImageCount = imageCount;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = surface_extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  queue_family_indices_t indices = _find_queue_families(g_vk_physical_device);
  uint32_t queue_family_indices[2] = {indices.gfx_family,
                                      indices.present_family};

  if (indices.gfx_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = 0;
  }
  create_info.flags = 0;
  create_info.preTransform = sc_details.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = surface_present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;
  create_info.pNext = 0;

  if (vkCreateSwapchainKHR(g_vk_device, &create_info, 0, &g_vk_swapchain) !=
      VK_SUCCESS) {
    ptia_panic("Failed to create swapchain");
  }
  vkGetSwapchainImagesKHR(g_vk_device, g_vk_swapchain,
                          &g_vk_swapchain_image_count, 0);
  g_vk_swapchain_images = malloc(sizeof(VkImage) * g_vk_swapchain_image_count);
  vkGetSwapchainImagesKHR(g_vk_device, g_vk_swapchain,
                          &g_vk_swapchain_image_count, g_vk_swapchain_images);

  g_vk_swapchain_extent = surface_extent;
  g_vk_swapchain_format = surface_format.format;
}

void _init_vk_image_views() {
  g_vk_image_view_count = g_vk_swapchain_image_count;
  g_vk_image_views = malloc(sizeof(VkImageView) * g_vk_image_view_count);

  for (size_t i = 0; i < g_vk_image_view_count; i++) {
    VkImageViewCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = g_vk_swapchain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = g_vk_swapchain_format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    create_info.pNext = 0;
    create_info.flags = 0;

    if (vkCreateImageView(g_vk_device, &create_info, 0, &g_vk_image_views[i]) !=
        VK_SUCCESS) {
      ptia_panic("failed to create image view");
    }
  }
}

VkShaderModule _create_shader_module(file_str_t code) {
  VkShaderModuleCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  printf("%d\n", (uint32_t)code.size);
  printf("%s\n", code.data);
  create_info.codeSize = code.size;
  create_info.pCode = (const uint32_t *)code.data;
  create_info.flags = 0;
  create_info.pNext  = 0;

  VkShaderModule module;
  if (vkCreateShaderModule(g_vk_device, &create_info, 0, &module) !=
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

  return create_info;
}

void _init_vk_graphics_pipeline() {
  file_str_t vert_shader_code = read_file("shaders/vert.spv");
  file_str_t frag_shader_code = read_file("shaders/frag.spv");

  g_vk_vert_shader = _create_shader_module(vert_shader_code);
  g_vk_frag_shader = _create_shader_module(frag_shader_code);

  VkPipelineShaderStageCreateInfo stages[] = {
      _create_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, g_vk_vert_shader,
                           "main"),
      _create_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, g_vk_frag_shader,
                           "main")};

  VkDynamicState dyn_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                 VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dyn_states_create_info;
  dyn_states_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dyn_states_create_info.dynamicStateCount = 2;
  dyn_states_create_info.pDynamicStates = dyn_states;

  VkPipelineVertexInputStateCreateInfo vtx_input_create_info;
  vtx_input_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vtx_input_create_info.vertexBindingDescriptionCount = 0;
  vtx_input_create_info.pVertexBindingDescriptions = 0;
  vtx_input_create_info.vertexAttributeDescriptionCount = 0;
  vtx_input_create_info.pVertexAttributeDescriptions = 0;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info;
  input_assembly_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport;
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = g_vk_swapchain_extent.width;
  viewport.height = g_vk_swapchain_extent.height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = g_vk_swapchain_extent;

  VkPipelineViewportStateCreateInfo vport_create_info;
  vport_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vport_create_info.viewportCount = 1;
  vport_create_info.scissorCount = 1;
  vport_create_info.pScissors = &scissor;
  vport_create_info.pViewports = &viewport;

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

  VkPipelineMultisampleStateCreateInfo multisample_create_info;
  multisample_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_create_info.sampleShadingEnable = VK_FALSE;
  multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_create_info.minSampleShading = 1.0f;
  multisample_create_info.pSampleMask = 0;
  multisample_create_info.alphaToCoverageEnable = VK_FALSE;
  multisample_create_info.alphaToOneEnable = VK_FALSE;

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

  VkPipelineLayoutCreateInfo pipeline_layout_create_info;
  pipeline_layout_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = 0;
  pipeline_layout_create_info.pSetLayouts = 0;
  pipeline_layout_create_info.pPushConstantRanges = 0;
  pipeline_layout_create_info.pushConstantRangeCount = 0;
  pipeline_layout_create_info.flags = 0;
  pipeline_layout_create_info.pNext = 0;
  if (vkCreatePipelineLayout(g_vk_device, &pipeline_layout_create_info, 0,
                             &g_vk_pipeline_layout) != VK_SUCCESS) {
    ptia_panic("Failed to create vk Pipeline Layout");
  }
}

void ptia_init_vk(const char *app_name, uint32_t app_version) {
  _init_glfw();
  printf("init stage 1\n");
  _init_vk_instance(app_name, app_version);
  printf("init stage 2\n");
  _preinit_vk_surface();
  printf("init stage 3\n");
  _init_vk_pick_phy_dev();
  printf("init stage 4\n");
  _init_vk_logical_device();
  printf("init stage 5\n");
  _create_swapchain();
  printf("init stage 6\n");
  _init_vk_image_views();
  printf("init stage 7\n");
  _init_vk_graphics_pipeline();
  printf("init stage 8\n");
}

VkInstance ptia_get_vk_instance() {
  if (g_vk_instance == VK_NULL_HANDLE) {
    ptia_panic("vk Instance not yet initialised");
  }
  return g_vk_instance;
}

void ptia_destroy_vk() {
  printf("Tearing down vk");
  vkDestroyPipelineLayout(g_vk_device, g_vk_pipeline_layout, 0);
  vkDestroyShaderModule(g_vk_device, g_vk_vert_shader, 0);
  vkDestroyShaderModule(g_vk_device, g_vk_frag_shader, 0);

  for (size_t i = 0; i < g_vk_image_view_count; i++) {
    vkDestroyImageView(g_vk_device, g_vk_image_views[i], 0);
  }

  vkDestroySwapchainKHR(g_vk_device, g_vk_swapchain, 0);
  vkDestroySurfaceKHR(g_vk_instance, g_vk_surface, 0);
  glfwTerminate();
  vkDestroyDevice(g_vk_device, 0);
  if (USE_VALIDATION_LAYERS) {
    DestroyDebugUtilsMessengerEXT(g_vk_instance, g_debug_messenger, 0);
  }
  vkDestroyInstance(g_vk_instance, 0);
}
