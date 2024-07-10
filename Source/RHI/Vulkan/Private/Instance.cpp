#include "Instance.h"

MKInstance::MKInstance() 
    : _validationLayer()
{
    // specify application create info
	VkApplicationInfo appInfo = vkinfo::GetApplicationInfo();
    auto extension = GetRequiredExtensions();
 
    // specify instance create info
#ifdef NDEBUG
	VkInstanceCreateInfo instanceInfo = vkinfo::GetInstanceCreateInfo(&appInfo, extension.size(), extension.data());
#else
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vkinfo::GetDebugMessengerCreateInfo(MKValidationLayer::DebugCallback);
    VkInstanceCreateInfo instanceInfo = vkinfo::GetInstanceCreateInfo(
        &appInfo, 
        extension.size(), 
        extension.data(), 
        (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo
    );
#endif

    // create VkInstance
    MK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &_vkInstance));

    // setup debug messenger
    _validationLayer.SetupDebugMessenger(_vkInstance);
}

MKInstance::~MKInstance()
{
#ifndef NDEBUG
    _validationLayer.DestroyDebugUtilsMessengerEXT(_vkInstance, nullptr);
    MK_LOG("debug utils messenger destroyed");
#endif

    vkDestroyInstance(_vkInstance, nullptr);

#ifndef NDEBUG
    MK_LOG("vulkan instance destroyed");
#endif
}

std::vector<const char*> MKInstance::GetRequiredExtensions()
{
    uint32 glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    extensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
    if (!CheckExtensionSupport(extensions)) 
    {
        throw std::runtime_error("Required extensions are not supported");
    }

#ifndef NDEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

std::vector<VkExtensionProperties> MKInstance::GetAvailableExtensions()
{
    uint32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
   
    return extensions;
}

bool MKInstance::CheckExtensionSupport(std::vector<const char*>& requiredExtensions)
{
    auto availableExtensions = GetAvailableExtensions();
    auto requiredSize = requiredExtensions.size();

    // check if required extensions are available
    // if avaialbe, decrement requiredSize
    for (const auto& availableExtension : availableExtensions) 
    {
        for (const auto& requiredExtenion : requiredExtensions) {
			if (strcmp(availableExtension.extensionName, requiredExtenion) == 0) {
				requiredSize--;
				break;
			}
        }
    }

    return requiredSize == 0;
}