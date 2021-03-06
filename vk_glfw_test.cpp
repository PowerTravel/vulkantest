#include <exception>
#include <stdexcept>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

const std::vector<char const *> validationLayers =
{
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<char const *> deviceExtensions =
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#ifdef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
GLFWwindow* window;
VkInstance Instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice Device = VK_NULL_HANDLE;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSurfaceKHR surface;
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFromat;
VkExtent2D swapChainExtent;
std::vector<VkImageView> swapChainImageViews;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout; // Not used yet
VkPipeline graphicsPipeline;
std::vector<VkFramebuffer> swapChainFramebuffers;

VkResult CreateDebugUtilsMessengerEXT( VkInstance instance,
   const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
   const VkAllocationCallbacks* pAllocator,
   VkDebugUtilsMessengerEXT* pDebugMessenger )
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }else{ 
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT( VkInstance instance,
   VkDebugUtilsMessengerEXT DebugMessenger,
   const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func) {
    return func(instance, debugMessenger, nullptr);
  }
}

bool checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers)
  {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers)
    {
      if (std::strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

void initWindow()
{
  glfwInit();

  // glfw was ment to be run with an OpenGL context
  // GLFW_CLIENT_API with GLFW_NO_API tells it not to do that
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

std::vector<const char*> getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char*>Result(glfwExtensions, glfwExtensions + glfwExtensionCount);
  if (enableValidationLayers)
    Result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return Result;
}

void printAvailableExtensions()
{
  // Check for available extentions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
  std::cout << "available extensions:\n";

  for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << std::endl;
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
  if( messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
  }
  return VK_FALSE;
}

void populateDebugMessagerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.pNext = 0;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;  
  createInfo.pUserData = nullptr;
}

void createInstance()
{
  if(enableValidationLayers && !checkValidationLayerSupport())
    throw std::runtime_error("Validatuion layers requested but not available");

  // An optional struct providing metadata about our program
  VkApplicationInfo ApplicationInfo{};
  ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  ApplicationInfo.pApplicationName = "Hello Triangle";
  ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  ApplicationInfo.pEngineName = "No Engine";
  ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

  printAvailableExtensions();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

  // Necessary struct
  VkInstanceCreateInfo CreateInstanceInfo{};
  CreateInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  CreateInstanceInfo.pApplicationInfo = &ApplicationInfo;

  // The global validation layers are specified here.
  // They can be used to inject code to validate parts stuff.
  if (enableValidationLayers) {
    CreateInstanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    CreateInstanceInfo.ppEnabledLayerNames = validationLayers.data();
    
    // Add debug callback to create and destroy vk instance functions.
    populateDebugMessagerCreateInfo(debugCreateInfo);
    CreateInstanceInfo.pNext = (void*) &debugCreateInfo;
  }else{
    CreateInstanceInfo.enabledLayerCount = 0;
    CreateInstanceInfo.ppEnabledLayerNames = 0;
  }
  
  const auto extensions = getRequiredExtensions();
  CreateInstanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  CreateInstanceInfo.ppEnabledExtensionNames = extensions.data();

  // The second argument can be a callback to a custom allocator
  if (vkCreateInstance(&CreateInstanceInfo, nullptr, &Instance) != VK_SUCCESS)
  {
    exit(1);
  }
}

void setupDebugMessenger()
{
  if (!enableValidationLayers)
    return;
  
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessagerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    throw std::runtime_error("Failed to set up debug messenger!");

}

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete()
  { 
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
  QueueFamilyIndices indices;
  uint32_t queueFamilyCount{};
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
  

  int i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if(presentSupport)
      indices.presentFamily = i;

    if(indices.isComplete())
      break;

    i++;
  }

  return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requeredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto& extension : availableExtensions)
    requeredExtensions.erase(extension.extensionName);

  return requeredExtensions.empty();
}


struct SwapCahinSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

SwapCahinSupportDetails querySwapChainSupportDetails(VkPhysicalDevice device)
{
  SwapCahinSupportDetails details{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
  if (presentModeCount)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

bool isDeviceSuitable(VkPhysicalDevice device)
{
#if 0
  // functions to query for supported device properties and features
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
#endif

  QueueFamilyIndices indices = findQueueFamilies(device);
  bool extensionSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionSupported)
  {
    SwapCahinSupportDetails swapChainSupport = querySwapChainSupportDetails(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionSupported && swapChainAdequate;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
  for (const auto& availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
  if(capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }else{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent {
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height)
    };
    
    actualExtent.width  = std::clamp(actualExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
  
    return actualExtent;
  }
}

void pickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());
  
  for (auto device : devices)
  {
    if (isDeviceSuitable(device))
    {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE)
    throw std::runtime_error("failed to find a suitable GPU");

}

void createLogicalDevice()
{ 
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueCueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueCueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // Empty for now
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers)
  {
    // Before there were separate validation layers between instance and device specific validation layers.
    // In modern implementations they are the same, but for compatibility's sake it's recommended to specify
    // them anyways.

    // Ignored by up-to-date implementations
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }else{
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS)
    throw std::runtime_error("failed to create logical device.");

  vkGetDeviceQueue(Device, indices.graphicsFamily.value(), 0, &graphicsQueue);

}

void createSurface()
{
  if (glfwCreateWindowSurface(Instance, window, nullptr, &surface) != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface.");
}

void createSwapChain()
{
  SwapCahinSupportDetails swapChainSupport = querySwapChainSupportDetails(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  // It is recommended to request at least one more image than the minimum
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    imageCount = swapChainSupport.capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily)
  {
    // if queuefamilies are not the same we let them share the swapchain.
    // This is less efficient than using VK_SHARING_MODE_EXCLUSIVE but then we would
    // have to transfer ownership of the swapchain explicitly, which is out of scope for
    // this  tutorial for now.
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }else{
    // if they are the same we can just set the sharing mode to exclusive
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // This means apply no transform (rotation, reflection etc...)
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do we want to blend the window with the os? NO!
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE; // ignores pixels that are obscured by other windows. Not good if you want to record windows for example but improves performance.
  createInfo.oldSwapchain = VK_NULL_HANDLE; // Resizing the image would require us to create a new swap chain and reference the old on here. This will be covered later.
  
  if (vkCreateSwapchainKHR(Device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    throw std::runtime_error("failed to create swap chain.");

  vkGetSwapchainImagesKHR(Device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(Device, swapChain, &imageCount, swapChainImages.data());
  swapChainImageFromat = surfaceFormat.format;
  swapChainExtent = extent;

}

void createImageViews()
{
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); ++i)
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFromat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // No swizzling
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Used as color target
    createInfo.subresourceRange.baseMipLevel = 0; // No MipMaping
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(Device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create image views");

  }
}

std::vector<char> readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if(!file.is_open())
    throw std::runtime_error("failed to open file");

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& code)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  // This pointer requires that data is aligned uin32_t* but our data is a unit8_t*
  // However, apparantly vector already aligns data for worst case (32bit) so we are fine.
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule{};
  vkCreateShaderModule(Device, &createInfo, nullptr, &shaderModule);
  return shaderModule;
}

void createGraphicsPipeline()
{

  // Shaders 

  auto vertShaderCode = readFile("shaders/vert.spv");
  auto fragShaderCode = readFile("shaders/frag.spv");
  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.pName = "main";
  fragShaderStageInfo.module = fragShaderModule;

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // Fixed functions

  // Vertex Buffer
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport
  VkViewport viewPort{};
  viewPort.x = 0.0f;
  viewPort.y = 0.0f;
  viewPort.width = (float) swapChainExtent.width;
  viewPort.height = (float) swapChainExtent.height;
  viewPort.minDepth = 0;
  viewPort.maxDepth = 1;

  VkRect2D scissor{};
  scissor.offset = {0,0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewPort;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

  // Multisampling (off, revisit later)
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional

  // Depth and Stencil testing(off, revisit later)

  // Color Blending
  // VkPipelineColorBlendStateCreateInfo is used for global color blending, but we only have one framebuffer so it's not needed

  // This creates standard alpha-blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional 
  colorBlending.blendConstants[1] = 0.0f; // Optional 
  colorBlending.blendConstants[2] = 0.0f; // Optional 
  colorBlending.blendConstants[3] = 0.0f; // Optional 

  // Dynamic State (Can be changed without recreating the pipeline, not used for now)
  VkDynamicState dynamicStates[] ={
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  // Pipeline Layout (used to define uniforms, not used for now, must still be created)
  
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout");

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;

  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0; // This pipeline will be used for the color render pass we defined
                            // Several renderpasses compatible with renderpass can be used, 
                            // but that functionality is out of scope.

  // Used to create a pipeline based on another pipeline if they have alot in common.
  // Requires flags field to have VK_PIPELINE_CREATE_DERIVATIVE_BIT set
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline");

  vkDestroyShaderModule(Device, vertShaderModule, nullptr);
  vkDestroyShaderModule(Device, fragShaderModule, nullptr);
}

void createRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFromat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the image before rendering;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the image after rendering so we can display it.
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't use stencil buffer atm
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Since we clear the image we don't care about the layout before rendering.
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Optimize layout for presenting on screen

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0; // Since we only have 1 colorAttachment, it's index will be 0
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // The index of this attachment is directly referenced in the shader code with the
  // 'layout (location = 0) out vec4 outColor' directive
  // Following types of attachments exist
  //
  // -  pInputAttachments: Attachments that are read from a shader
  // -  pResolveAttachments: Attachments used for multisampling color attachments
  // -  pDepthStencilAttachment: Attachment for depth and stencil data
  // -  pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
  VkSubpassDescription subpass{};
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (vkCreateRenderPass(Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass.");
    
  
}

void createFrameBuffers()
{
  swapChainFramebuffers.resize(swapChainImageViews.size());
  for (size_t i = 0; i < swapChainFramebuffers.size(); ++i)
  {
    VkImageView attachments[] = {
      swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if(vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create framebuffer.");
  }
}

void initVulkan()
{
  // Take all notes with a fist of salt, Im still learning.
  createInstance();         // Create a Vulkan instance
  setupDebugMessenger();    // Set up debug messengers 
  createSurface();          // Create a render surface, basically a glfw window with a vulkan context.
  pickPhysicalDevice();     // Choose graphics card
  createLogicalDevice();    // Configure the capabilities of the card
  createSwapChain();        // Create a chain of images to displat
  createImageViews();       // Configure each image in the chain
  createRenderPass();       // Structure referenced by the pipeline
  createGraphicsPipeline(); // Set up buffers, renderstate, blending etc
  createFrameBuffers();     // Binds together VkImageViews retrieved from the swapChain and RenderPassAttachments
}

void cleanup()
{
  for (auto framebuffer : swapChainFramebuffers)
    vkDestroyFramebuffer(Device, framebuffer, nullptr);

  vkDestroyPipeline(Device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);
  vkDestroyRenderPass(Device, renderPass, nullptr);
  for (auto imageView : swapChainImageViews)
    vkDestroyImageView(Device, imageView, nullptr);
  vkDestroySwapchainKHR(Device,swapChain,nullptr);
  vkDestroyDevice(Device, nullptr);
  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(Instance, debugMessenger, nullptr);
  }
  vkDestroySurfaceKHR(Instance, surface, nullptr);
  vkDestroyInstance(Instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void mainLoop()
{
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }
}

void run()
{
  try{
    initWindow();
    initVulkan();
  }catch(const std::exception& e){
    std::cerr << e.what() << std::endl;
    return;
  }
  mainLoop();
  cleanup();
}


int main( int argc, char* argv[])
{
  glm::mat4 matrix;
  glm::vec4 vec;
  auto test = matrix * vec;

  run();

  return 0;
}
