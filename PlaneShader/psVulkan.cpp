// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psVulkan.h"
#include "psEngine.h"

using namespace planeshader;

#ifdef BSS_CPU_x86_64
#pragma comment(lib, "../lib/vulkan-1.lib")
#else
#pragma comment(lib, "../lib/vulkan32-1.lib")
#endif

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint) {                        \
    fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
    if (fp##entrypoint == NULL) PSLOG(2) << "vkGetInstanceProcAddr failed to find vk" #entrypoint << std::endl; \
  }

psVulkan::psVulkan(const psVeciu& dim, uint32_t antialias, bool vsync, bool fullscreen, bool sRGB, psMonitor* monitor) : psDriver()
{ // Note: can't use memset() because it destroys all the psDriver stuff.
  _instance = 0;
  _gpu = 0;
  _device = 0;
  _queue = 0;
  _surface = 0;
  _swapchain = 0;
  enabled_extension_count = 0;
  enabled_layer_count = 0;
  graphics_queue_node_index = 0;
  memset(extension_names, 0, sizeof(extension_names));
  queue_count = 0;
  _backbuffer = 0;

  VkResult err;
  uint32_t instance_extension_count = 0;
  uint32_t instance_layer_count = 0;
  uint32_t validation_layer_count = 0;
  char **instance_validation_layers = NULL;

  /* Look for instance extensions */
  VkBool32 surfaceExtFound = 0;
  VkBool32 platformSurfaceExtFound = 0;

  err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
  assert(!err);

  if(instance_extension_count > 0)
  {
    std::unique_ptr<VkExtensionProperties[]> instance_extensions(new VkExtensionProperties[instance_extension_count]);
    err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions.get());
    assert(!err);
    for(uint32_t i = 0; i < instance_extension_count; i++)
    {
      if(!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
      {
        surfaceExtFound = 1;
        extension_names[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
      }
      if(!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
      {
        platformSurfaceExtFound = 1;
        extension_names[enabled_extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
      }
      assert(enabled_extension_count < 64);
    }
  }

  if(!surfaceExtFound)
    PSLOG(1) << "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME " extension" << std::endl;
  if(!platformSurfaceExtFound)
    PSLOG(1) << "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME " extension" << std::endl;
  const VkApplicationInfo app = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    NULL,
    "PlaneShader",
    0,
    "PlaneShader",
    0,
    VK_API_VERSION_1_0,
  };
  VkInstanceCreateInfo inst_info = {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    NULL,
    0,
    &app,
    enabled_layer_count,
    (const char *const *)instance_validation_layers,
    enabled_extension_count,
    (const char *const *)extension_names,
  };

  uint32_t gpu_count;

  err = vkCreateInstance(&inst_info, NULL, &_instance);
  if(err)
  {
    PSLOG(1) << "vkCreateInstance failed with error: " << VkResultToString(err) << std::endl;
    return;
  }

  /* Make initial call to query gpu_count, then second call for gpu info*/
  err = vkEnumeratePhysicalDevices(_instance, &gpu_count, NULL);
  assert(!err && gpu_count > 0);

  if(gpu_count > 0)
  {
    std::unique_ptr<VkPhysicalDevice[]> physical_devices(new VkPhysicalDevice[gpu_count]);
    err = vkEnumeratePhysicalDevices(_instance, &gpu_count, physical_devices.get());
    assert(!err);
    /* For tri demo we just grab the first physical device */
    _gpu = physical_devices[0];
  }
  else
  {
    PSLOG(1) << "vkEnumeratePhysicalDevices reported zero accessible devices!" << std::endl;
    return;
  }

  /* Look for device extensions */
  uint32_t device_extension_count = 0;
  VkBool32 swapchainExtFound = 0;
  enabled_extension_count = 0;
  memset(extension_names, 0, sizeof(extension_names));

  err = vkEnumerateDeviceExtensionProperties(_gpu, NULL, &device_extension_count, NULL);
  assert(!err);

  if(device_extension_count > 0)
  {
    std::unique_ptr<VkExtensionProperties[]> device_extensions(new VkExtensionProperties[device_extension_count]);
    err = vkEnumerateDeviceExtensionProperties(_gpu, NULL, &device_extension_count, device_extensions.get());
    assert(!err);

    for(uint32_t i = 0; i < device_extension_count; i++)
    {
      if(!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName))
      {
        swapchainExtFound = 1;
        extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
      }
      assert(enabled_extension_count < 64);
    }
  }

  if(!swapchainExtFound)
    PSLOG(1) << "vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME " extension" << std::endl;

  // Having these GIPA queries of device extension entry points both
  // BEFORE and AFTER vkCreateDevice is a good test for the loader
  GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
  GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfaceFormatsKHR);
  GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfacePresentModesKHR);
  GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfaceSupportKHR);
  GET_INSTANCE_PROC_ADDR(_instance, CreateSwapchainKHR);
  GET_INSTANCE_PROC_ADDR(_instance, DestroySwapchainKHR);
  GET_INSTANCE_PROC_ADDR(_instance, GetSwapchainImagesKHR);
  GET_INSTANCE_PROC_ADDR(_instance, AcquireNextImageKHR);
  GET_INSTANCE_PROC_ADDR(_instance, QueuePresentKHR);

  vkGetPhysicalDeviceProperties(_gpu, &_device_props);

  // Query with NULL data to get count
  vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queue_count, NULL);

  queue_props = std::unique_ptr<VkQueueFamilyProperties[]>(new VkQueueFamilyProperties[queue_count]);
  vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queue_count, queue_props.get());
  assert(queue_count >= 1);

  vkGetPhysicalDeviceFeatures(_gpu, &_device_features);

  _prepare(fullscreen, sRGB, monitor);
  _loadswapchain(dim, vsync);
  _createcommands();
  _driver = this;

  // DEBUG ONLY


  psShader* nullshader = psShader::CreateShader(0, 0, 0, 0);
  _backbuffer = new psTex(psVeciu(0, 0), FMT_UNKNOWN, 0, 0, 0, psVeciu(0, 0));
  library.IMAGE0 = nullshader;
  library.IMAGE = nullshader;
  library.IMAGE2 = nullshader;
  library.IMAGE3 = nullshader;
  library.IMAGE4 = nullshader;
  library.IMAGE5 = nullshader;
  library.IMAGE6 = nullshader;
  library.CIRCLE = nullshader;
  library.POLYGON = nullshader;
  library.LINE = nullshader;
  library.PARTICLE = nullshader;
  library.TEXT1 = nullshader;
  library.DEBUG = nullshader;
  library.CURVE = nullshader;
  library.ROUNDRECT = nullshader;
}

psVulkan::~psVulkan()
{
  vkFreeCommandBuffers(_device, _cmdpool, 1, &_cmdbuffer);
  vkDestroyCommandPool(_device, _cmdpool, NULL);
  fpDestroySwapchainKHR(_device, _swapchain, NULL);
  vkDestroyDevice(_device, NULL);
  vkDestroySurfaceKHR(_instance, _surface, NULL);
  vkDestroyInstance(_instance, NULL);
}

void psVulkan::_createdevice()
{
  VkResult err;

  float queue_priorities[1] = { 0.0 };
  const VkDeviceQueueCreateInfo queue = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    NULL,
    0,
    graphics_queue_node_index,
    1,
    queue_priorities
  };
  
  VkPhysicalDeviceFeatures features;
  memset(&features, 0, sizeof(features));
  if(_device_features.shaderClipDistance)
    features.shaderClipDistance = VK_TRUE;

  VkDeviceCreateInfo device = {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    NULL,
    0,
    1,
    &queue,
    0,
    NULL,
    enabled_extension_count,
    (const char *const *)extension_names,
    &features,
  };

  err = vkCreateDevice(_gpu, &device, NULL, &_device);
  assert(!err);
}

void psVulkan::_prepare(bool fullscreen, bool sRGB, psMonitor* monitor)
{
  VkResult err;
  uint32_t i;

  VkWin32SurfaceCreateInfoKHR createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.hinstance = GetModuleHandleW(0);
  createInfo.hwnd = monitor->GetWindow();

  err = vkCreateWin32SurfaceKHR(_instance, &createInfo, NULL, &_surface);
  if(err)
  {
    PSLOG(1) << "vkCreateWin32SurfaceKHR failed with error " << VkResultToString(err) << std::endl;
    return;
  }
  vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queue_count, NULL);
  // Iterate over each queue to learn whether it supports presenting:
  std::unique_ptr<VkBool32[]> supportsPresent(new VkBool32[queue_count]);
  for(i = 0; i < queue_count; i++)
    fpGetPhysicalDeviceSurfaceSupportKHR(_gpu, i, _surface, &supportsPresent[i]);

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
  uint32_t graphicsQueueNodeIndex = UINT32_MAX;
  uint32_t presentQueueNodeIndex = UINT32_MAX;
  for(i = 0; i < queue_count; i++)
  {
    if((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
    {
      if(graphicsQueueNodeIndex == UINT32_MAX)
      {
        graphicsQueueNodeIndex = i;
      }

      if(supportsPresent[i] == VK_TRUE)
      {
        graphicsQueueNodeIndex = i;
        presentQueueNodeIndex = i;
        break;
      }
    }
  }
  if(presentQueueNodeIndex == UINT32_MAX)
  {
    // If didn't find a queue that supports both graphics and present, then
    // find a separate present queue.
    for(uint32_t i = 0; i < queue_count; ++i)
    {
      if(supportsPresent[i] == VK_TRUE)
      {
        presentQueueNodeIndex = i;
        break;
      }
    }
  }

  // Generate error if could not find both a graphics and a present queue
  if(graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
  {
    PSLOG(1) << "Could not find a graphics and a present queue" << std::endl;
    return;
  }

  // We assume that our presentation and graphics queues are the same, but they don't have to be.
  if(graphicsQueueNodeIndex != presentQueueNodeIndex)
  {
    PSLOG(1) << "Could not find a common graphics and a present queue" << std::endl;
    return;
  }

  graphics_queue_node_index = graphicsQueueNodeIndex;

  _createdevice();
  vkGetDeviceQueue(_device, graphics_queue_node_index, 0, &_queue);

  // Get the list of VkFormat's that are supported:
  uint32_t formatCount;
  err = fpGetPhysicalDeviceSurfaceFormatsKHR(_gpu, _surface, &formatCount, NULL);
  assert(!err);
  std::unique_ptr<VkSurfaceFormatKHR[]> surfFormats(new VkSurfaceFormatKHR[formatCount]);
  err = fpGetPhysicalDeviceSurfaceFormatsKHR(_gpu, _surface, &formatCount, surfFormats.get());
  assert(!err);
  // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
  // the surface has no preferred format.  Otherwise, at least one
  // supported format will be returned.
  if(formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    _format = VK_FORMAT_B8G8R8A8_UNORM;
  else
  {
    assert(formatCount >= 1);
    _format = surfFormats[0].format;
  }
  _colorspace = surfFormats[0].colorSpace;

  // Get Memory information and properties
  vkGetPhysicalDeviceMemoryProperties(_gpu, &_memory_props);
}
void psVulkan::_loadswapchain(const psVeciu& dim, bool vSync)
{
  VkResult err;

  // Check the surface capabilities and formats
  VkSurfaceCapabilitiesKHR surfCapabilities;
  err = fpGetPhysicalDeviceSurfaceCapabilitiesKHR(_gpu, _surface, &surfCapabilities);
  assert(!err);

  uint32_t presentModeCount;
  err = fpGetPhysicalDeviceSurfacePresentModesKHR(_gpu, _surface, &presentModeCount, NULL);
  assert(!err);
  std::unique_ptr<VkPresentModeKHR[]> presentModes(new VkPresentModeKHR[presentModeCount]);
  err = fpGetPhysicalDeviceSurfacePresentModesKHR(_gpu, _surface, &presentModeCount, presentModes.get());
  assert(!err);

  VkExtent2D swapchainExtent;
  // width and height are either both -1, or both not -1.
  if(surfCapabilities.currentExtent.width == (uint32_t)-1)
  {
    // If the surface size is undefined, the size is set to
    // the size of the images requested.
    swapchainExtent.width = dim.x;
    swapchainExtent.height = dim.y;
  }
  else
  {
    // If the surface size is defined, the swap chain size must match
    swapchainExtent = surfCapabilities.currentExtent;
    //demo->width = surfCapabilities.currentExtent.width;
    //demo->height = surfCapabilities.currentExtent.height;
  }

  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  // Determine the number of VkImage's to use in the swap chain (we desire to
  // own only 1 image at a time, besides the images being displayed and
  // queued for display):
  uint32_t desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
  if((surfCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount))
    desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;

  VkSurfaceTransformFlagBitsKHR preTransform = (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfCapabilities.currentTransform;

  const VkSwapchainCreateInfoKHR swapchain = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    NULL,
    0,
    _surface,
    desiredNumberOfSwapchainImages,
    _format,
    _colorspace,
    { swapchainExtent.width, swapchainExtent.height, },
    1,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    VK_SHARING_MODE_EXCLUSIVE,
    0,
    NULL,
    preTransform,
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    swapchainPresentMode,
    true,
    _swapchain,
  };

  VkSwapchainKHR old = _swapchain;
  err = fpCreateSwapchainKHR(_device, &swapchain, NULL, &_swapchain);
  assert(!err);

  if(old != VK_NULL_HANDLE)
    fpDestroySwapchainKHR(_device, old, NULL);


  err = fpGetSwapchainImagesKHR(_device, _swapchain, &_swapchainImageCount, NULL);
  assert(!err);

  std::unique_ptr<VkImage[]> swapchainImages(new VkImage[_swapchainImageCount]);
  err = fpGetSwapchainImagesKHR(_device, _swapchain, &_swapchainImageCount, swapchainImages.get());
  assert(!err);

  _buffers = std::unique_ptr<SwapChainBuffer[]>(new SwapChainBuffer[_swapchainImageCount]);

  for(uint32_t i = 0; i < _swapchainImageCount; i++)
  {
    VkImageViewCreateInfo color_attachment_view = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        NULL,
        0,
        0,
        VK_IMAGE_VIEW_TYPE_2D,
        _format,
      {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
      },
      { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };

    _buffers[i].image = swapchainImages[i];

    color_attachment_view.image = _buffers[i].image;

    err = vkCreateImageView(_device, &color_attachment_view, NULL,
      &_buffers[i].view);
    assert(!err);
  }

  //_current_buffer = 0;
}
void psVulkan::_createcommands()
{
  VkResult err;

  const VkCommandPoolCreateInfo cmd_pool_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    NULL,
    graphics_queue_node_index,
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
  };
  err = vkCreateCommandPool(_device, &cmd_pool_info, NULL, &_cmdpool);
  assert(!err);

  const VkCommandBufferAllocateInfo cmd = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    NULL,
    _cmdpool,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    1,
  };
  err = vkAllocateCommandBuffers(_device, &cmd, &_cmdbuffer);
  assert(!err);
}
const char* psVulkan::VkResultToString(const VkResult result)
{
  //static char buf[64] = "UNKNOWN ERROR";

  switch(result)
  {
    case VK_SUCCESS: return "SUCCESS";
    case VK_NOT_READY: return "NOT_READY";
    case VK_TIMEOUT: return "TIMEOUT";
    case VK_EVENT_SET: return "EVENT_SET";
    case VK_EVENT_RESET: return "EVENT_RESET";
    case VK_INCOMPLETE: return "INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_SURFACE_LOST_KHR: return "ERROR_SURFACE_LOST";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "ERROR_NATIVE_WINDOW_IN_USE";
    case VK_SUBOPTIMAL_KHR: return "SUBOPTIMAL";
    case VK_ERROR_OUT_OF_DATE_KHR: return "ERROR_OUT_OF_DATE";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "ERROR_INCOMPATIBLE_DISPLAY";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "ERROR_VALIDATION_FAILED_EXT";
  }

  return "UNKNOWN";
}