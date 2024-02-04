#include "MKValidationLayer.h"

MKValidationLayer::MKValidationLayer()
    : _debugMessenger(VK_NULL_HANDLE)
{
    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available.");
	}
}

MKValidationLayer::~MKValidationLayer() {}

bool MKValidationLayer::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // get all available layers

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) { // if layer name is same as requested
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

void MKValidationLayer::SetupDebugMessenger(VkInstance instance)
{
    if (!ENABLE_VALIDATION_LAYERS) return;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
    PopulateDebugMessengerCreateInfo(debugMessengerInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger");
    }
}

VkResult MKValidationLayer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	auto createProxyFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (createProxyFunc != nullptr) {
        return createProxyFunc(instance, pCreateInfo, pAllocator, &_debugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void MKValidationLayer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

void MKValidationLayer::DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    auto destroyProxyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyProxyFunc != nullptr) {
        destroyProxyFunc(instance, _debugMessenger, pAllocator);
    }
}
