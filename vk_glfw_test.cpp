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

const std::vector<char const *> validationLayers =
{
  "VK_LAYER_KHRONOS_validation"
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
  bool isComplete()
  { 
    return graphicsFamily.has_value();
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
    {
      indices.graphicsFamily = i;
      return indices;
    }
    i++;
  }
  return {};
}


bool isDeviceSuitable(VkPhysicalDevice device)
{
  bool result = false;
#if 0
  // Example of checking device capabilities
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  result = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader
#else 
  QueueFamilyIndices indices = findQueueFamilies(device);

  result = indices.isComplete();
#endif

  return result;
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

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // Empty for now
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.queueCreateInfoCount = 1;

  createInfo.enabledExtensionCount = 0;
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

void initVulkan()
{
  createInstance();
  setupDebugMessenger();
  pickPhysicalDevice();
  createLogicalDevice();
}

void cleanup()
{
  vkDestroyDevice(Device, nullptr);
  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(Instance, debugMessenger, nullptr);
  }
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
