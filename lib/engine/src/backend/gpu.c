#include "backend/gpu.h"
#include "GLFW/glfw3.h"
#include "backend/util.h"
#include "backend/window.h"
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
static VkFramebuffer *g_vk_swapchain_framebuffers = 0;
static VkRenderPass g_vk_render_pass = VK_NULL_HANDLE;

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
  device_create_info.ppEnabledExtensionNames =
      (const char **)g_device_extensions;
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
  uint32_t width;
  uint32_t height;
  window_get_resolution(&width, &height);
  char *title = window_get_title();
  g_wnd = glfwCreateWindow(width, height, title, 0, 0);
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

void _init_vk_render_passes() {

  VkAttachmentDescription color_attachment;
  color_attachment.flags = 0;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.format = gpu_get_swapchain_format();
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment_ref.attachment = 0;

  VkSubpassDescription subpass;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.flags = 0;
  subpass.inputAttachmentCount = 0;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = 0;
  subpass.pInputAttachments = 0;
  subpass.pResolveAttachments = 0;
  subpass.pDepthStencilAttachment = 0;

  VkRenderPassCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.pNext = 0;
  create_info.flags = 0;
  create_info.attachmentCount = 1;
  create_info.pAttachments = &color_attachment;
  create_info.subpassCount = 1;
  create_info.pSubpasses = &subpass;
  create_info.dependencyCount= 0;
  create_info.pDependencies = 0;

  if (vkCreateRenderPass(g_vk_device, &create_info, 0, &g_vk_render_pass) !=
      VK_SUCCESS) {
    ptia_panic("Failed to create Render Pass");
  }
}

void _init_vk_framebuffers() {
  g_vk_swapchain_framebuffers =
      malloc(sizeof(VkFramebuffer) * g_vk_swapchain_image_count);

  for (uint32_t i = 0; i < g_vk_swapchain_image_count; i++) {
    VkImageView attachments[] = {g_vk_image_views[i]};

    VkFramebufferCreateInfo framebuffer_create_info;
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = 0;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = g_vk_render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.pAttachments = attachments;
    framebuffer_create_info.width = g_vk_swapchain_extent.width;
    framebuffer_create_info.height = g_vk_swapchain_extent.height;
    framebuffer_create_info.layers = 1;

    if (vkCreateFramebuffer(g_vk_device, &framebuffer_create_info, 0,
                            &g_vk_swapchain_framebuffers[i]) != VK_SUCCESS) {
      ptia_panic("Failed to create frame buffer");
    }
  }
}

void gpu_init_vk(const char *app_name, uint32_t app_version) {
  _init_glfw();
  printf("init vk instance\n");
  _init_vk_instance(app_name, app_version);
  printf("pre-init vk surface\n");
  _preinit_vk_surface();
  printf("pick from physical devices\n");
  _init_vk_pick_phy_dev();
  printf("init vk logical device \n");
  _init_vk_logical_device();
  printf("create initial swapchain\n");
  _create_swapchain();
  printf("init vk image views\n");
  _init_vk_image_views();
  printf("init vk pipeline render pass(es)\n");
  _init_vk_render_passes();
  printf("init vk framebuffers from render passes\n");
  _init_vk_framebuffers();
}

VkInstance gpu_get_vk_instance() { return g_vk_instance; }

VkDevice gpu_get_vk_device() { return g_vk_device; }

VkPhysicalDevice gpu_get_vk_phy_device() { return g_vk_physical_device; }

VkSwapchainKHR gpu_get_vk_swapchain() { return g_vk_swapchain; }

VkExtent2D gpu_get_swapchain_extent() { return g_vk_swapchain_extent; }

VkFormat gpu_get_swapchain_format() { return g_vk_swapchain_format; }

VkRenderPass gpu_get_render_pass() { return g_vk_render_pass; }

void gpu_destroy_vk() {
  printf("Tearing down vk\n");
  for (uint32_t i = 0; i < g_vk_swapchain_image_count; i++) {
    vkDestroyFramebuffer(g_vk_device, g_vk_swapchain_framebuffers[i], 0);
  }
  vkDestroyRenderPass(g_vk_device, g_vk_render_pass, 0);
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
